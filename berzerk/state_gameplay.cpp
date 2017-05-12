#include "state_gameplay.h"
#include "state_highscore.h"
#include "state_titlescreen.h"
#include "pugixml.hpp"

Directions StateGameplay::lastMove = Directions::W;

bool StateGameplay::chicken = false;

StateGameplay::StateGameplay( Game *game, bool recordDemo , bool playDemo )
{
	this->game = game;

	player.SetPos( maze.GetPlayerStart(lastMove, player) );
	transition = false;
	captured = false;

	// Init entities 
	entityManager.game = game;
	entityManager.Add( &player );
	AssetManager *assetManager = &this->game->assetManager;

	wallsCreated = false;
	enemiesSpawned = false;

	txScore.setFont( assetManager->GetFontRef( "joystix" ) );
#ifdef OLD_SFML
	txScore.setColor( sf::Color::Green );
#else
	txScore.setFillColor( sf::Color::Green );
#endif
	txScore.setCharacterSize( 30 );
	txScore.setPosition( sf::Vector2f( 3, 3 ) );
	txScore.setString( game->score );

	for( unsigned int i = 0; i < MAX_LIVES; i++ )
	{
		lives[i].setTexture( game->assetManager.GetTextureRef( "sprites" ) );
		lives[i].setTextureRect( sf::IntRect( 89, 0, 5, 8 ) );
		lives[i].setScale( 4, 4 );
		lives[i].setPosition( (5 + lives[i].getGlobalBounds().width) * i, GAME_HEIGHT - lives[i].getGlobalBounds().height - 5 );
	}

	ottoSpawned = false;

	deathSoundPlayed = false;

	clock.restart();

	this->recordDemo = recordDemo;
	this->playDemo = playDemo;
	if( playDemo )
		demo.LoadFromFile( game->GetConfigDir() + "demo1.xml" );
}

void StateGameplay::HandleInput()
{
	sf::Event event;

	// Reset state if player is dead
	if( player.CheckReset() )
	{
		game->ResetState();
		return;
	}

	while( game->window.pollEvent( event ) )
	{
		// Close window
		if( event.type == sf::Event::Closed )
			game->window.close();

		if( event.type == sf::Event::KeyPressed )
		{
			if( event.key.code == sf::Keyboard::Escape )
			{
				game->PopState();
				break; // If you don't break here, it will crash
			}

			if( event.key.code == sf::Keyboard::Key::F12 )
			{
				this->game->AddLife();
			}
			if( event.key.code == sf::Keyboard::Key::F11 )
			{
				transition = true;
				lastMove = Directions::N;
			}
			if( event.key.code == sf::Keyboard::Key::F10 )
			{
				this->game->SwitchState( new StateGameplay( this->game ) );
				break;
			}
			if( event.key.code == sf::Keyboard::Key::F9 )
			{
				demo.SaveToFile( game->GetConfigDir() + "demo1.xml" );
				txScore.setString( "Demo Saved" );
			}
		}

		if( event.type == sf::Event::MouseButtonPressed )
		{
			/*if( event.mouseButton.button == sf::Mouse::Left )
			{
				input.fire = true;
			}*/
		}

		if( event.type == sf::Event::MouseButtonReleased )
		{
			/*if( event.mouseButton.button == sf::Mouse::Left )
			{
				input.fire = false;
			}*/
		}

		// Early out if we're in the middle of a transition
		if( transition )
		{
			return;
		}

		// Keys presed
		if( game->inputManager.TestKeyDown( "right", event ) ) input.right = true;

		if( game->inputManager.TestKeyDown( "left", event ) ) input.left = true;

		if( game->inputManager.TestKeyDown( "up", event ) ) input.up = true;

		if( game->inputManager.TestKeyDown( "down", event ) ) input.down = true;

		if( game->inputManager.TestKeyDown( "fire", event ) ) input.fire = true;

		// Keys released
		if( game->inputManager.TestKeyUp( "right", event ) ) input.right = false;

		if( game->inputManager.TestKeyUp( "left", event ) ) input.left = false;

		if( game->inputManager.TestKeyUp( "up", event ) ) input.up = false;
		
		if( game->inputManager.TestKeyUp( "down", event ) ) input.down = false;

		if( game->inputManager.TestKeyUp( "fire", event ) ) input.fire = false;
	}

	if( playDemo && !recordDemo )
	{
		player.SetInput( demo.Play() );
	}
	else if( !playDemo )
	{
		player.SetInput( input );
		if( recordDemo ) // This is here so you can't record while playing a demo
			demo.Record( input );
	}
}

void StateGameplay::Update( const float dt )
{

	while ( !maze.IsDone() )
	{
		maze.Generate();
	}
	if ( maze.IsDone() && !wallsCreated )
	{
		if ( recordDemo )
			demo.SetWalls( maze.CreateWalls( entityManager ) );
		else if( playDemo )
			maze.LoadWalls( demo.GetWalls(), entityManager );
		else
			maze.CreateWalls( entityManager );

		if ( game->level >= 5 )
		{
			maze.BlockExit( entityManager, lastMove );
		}
		wallsCreated = true;
	}
	if( maze.IsDone() && !enemiesSpawned )
	{
		if ( recordDemo )
			demo.SetRobotPositions( maze.SpawnEnemies( entityManager, lastMove, LoadRobotStats() ) );
		else if ( playDemo )
			maze.LoadEnemies( demo.GetRobotPositions(), entityManager, LoadRobotStats() );
		else
			maze.SpawnEnemies( entityManager, lastMove, LoadRobotStats() );
		enemiesSpawned = true;

		sfx.setBuffer( game->assetManager.GetSoundRef( "humanoid", true ) );
		sfx.play();
	}

	entityManager.Think( dt );

	if( !transition )
	{
		entityManager.CheckCollisions();
		txScore.setString( std::to_string( game->score ) );

		// Begin screen transition if player moves outside screen
		sf::Vector2f plPos( player.hitbox.left, player.hitbox.top );
		sf::Int32 now = clock.getElapsedTime().asMilliseconds();
		if( plPos.x > GAME_WIDTH )
		{
			transition = true;
			game->level++;
			lastMove = Directions::W;
			transStart = now;
		}
		else if( ( plPos.x + player.hitbox.width ) < 0 )
		{
			transition = true;
			game->level++;
			lastMove = Directions::E;
			transStart = now;
		}
		else if( plPos.y > GAME_HEIGHT )
		{
			transition = true;
			game->level++;
			lastMove = Directions::N;
			transStart = now;
		}
		else if( ( plPos.y + player.hitbox.height ) < 0 )
		{
			transition = true;
			game->level++;
			lastMove = Directions::S;
			transStart = now;
		}

		// Did we run out of lives?
		if( game->GetLives() == 0 )
		{
			this->game->SwitchState( new StateHighscore( this->game ) );
			return;
		}

		// Spawn Otto if delay has been reached
		if( now >= ottoDelay && !ottoSpawned )
		{
			otto = new EntityOtto( sf::Vector2f( 0, player.hitbox.top ), player.hitbox.top, player.hitbox.top + player.hitbox.height );
			entityManager.Add( otto );
			ottoSpawned = true;

			sfx.setBuffer( game->assetManager.GetSoundRef( "intruder_alert", true ) );
			sfx.play();
		}
		// Tell Otto where to move
		if( ottoSpawned )
		{
			otto->SetMinMaxHeight( player.hitbox.top, player.hitbox.top + player.hitbox.height );
		}

		if( player.IsDead() && !deathSoundPlayed )
		{
			if( chicken )
			{
				sfx.setBuffer( game->assetManager.GetSoundRef( "got_chicken", true ) );
			}
			else
			{
				sfx.setBuffer( game->assetManager.GetSoundRef( "got_humanoid", true ) );
			}
			sfx.play();

			deathSoundPlayed = true;
		}
	}
	else
	{
		ScreenTransition(dt);
	}
}

void StateGameplay::Draw() const
{
	if( transition && captured )
	{
		game->window.draw( sprTrans );
		return; // Don't draw anything else
	}

	entityManager.Draw();
	game->window.draw( txScore );
	for( unsigned int i = 0; i < MAX_LIVES; i++ )
	{
		if( i >= game->GetLives() )
			break;

		game->window.draw( lives[i] );
	}
}

void StateGameplay::ScreenTransition( const float dt )
{
	if( !captured )
	{
		// Capture the screen
		txTrans.create( GAME_WIDTH, GAME_HEIGHT );
		txTrans.update( game->window );
		sprTrans.setTexture( txTrans );
		captured = true;

		// Play sound
		if( entityManager.GetRobotCount() > 0 ) chicken = true;
		else chicken = false;

		if( chicken )
		{
			sfx.setBuffer( game->assetManager.GetSoundRef( "chicken", true ) );
		}
		else
		{
			sfx.setBuffer( game->assetManager.GetSoundRef( "intruder", true ) );
		}
		sfx.play();
	}
	else
	{
		sf::Vector2f move;
		switch( lastMove )
		{
		case Directions::N:
			move = sf::Vector2f( 0, -TRANS_SPEED );
			break;
		case Directions::E:
			move = sf::Vector2f( TRANS_SPEED, 0 );
			break;
		case Directions::S:
			move = sf::Vector2f( 0, TRANS_SPEED );
			break;
		case Directions::W:
			move = sf::Vector2f( -TRANS_SPEED, 0 );
			break;
		}

		sprTrans.move( move * dt );

		// Check for done
		sf::FloatRect sprBounds = sprTrans.getGlobalBounds();
		if( ( sprBounds.left + sprBounds.width ) < 0 || sprBounds.left > GAME_WIDTH ||
			( sprBounds.top + sprBounds.height ) < 0 || sprBounds.top > GAME_HEIGHT )
		{
			this->game->SwitchState( new StateGameplay( this->game ) );
		}
	}
}

RobotStats StateGameplay::LoadRobotStats()
{
	RobotStats stats{ false, true, 50, 3000, 5, ERROR_COLOR }; // Default values in case of error

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file( "assets/robotstats.xml" );
	if( !result ) // Error check
	{
		return stats;
	}

	pugi::xml_node levelNodes = doc.child( "levels" );

	for( pugi::xml_node level : levelNodes.children( "level" ) )
	{
		if( game->level >= (unsigned int)std::stoi( level.attribute( "min" ).value() ) )
		{
			if( std::stoi( level.attribute( "stop_if_see_player" ).value() ) == 0 )
				stats.stopIfSeePlayer = false;
			else
				stats.stopIfSeePlayer = true;

			stats.movementSpeed = (float)std::stoi( level.attribute( "speed" ).value() );
			stats.fireDelay = std::stoi( level.attribute( "firedelay" ).value() );
			stats.numRobots = std::stoi( level.attribute( "num_bots" ).value() );

			if( level.attribute( "can_shoot" ).value() == "0" )
				stats.canShoot = false;
			else
				stats.canShoot = true;

			pugi::xml_node color = level.child( "color" );
			int r = std::stoi( color.attribute( "r" ).value() );
			int g = std::stoi( color.attribute( "g" ).value() );
			int b = std::stoi( color.attribute( "b" ).value() );
			stats.color = sf::Color( r, g, b );
		}
	}

	return stats;
}

StateGameplay::~StateGameplay()
{
	//maze.ClearMap();
	sfx.stop();
}