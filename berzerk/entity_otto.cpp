#include "entity_otto.h"

EntityOtto::EntityOtto( sf::Vector2f pos, const float minHeight, const float maxHeight, const bool dopefish, const int vDirection )
{
	hitbox.left = pos.x;
	hitbox.top = pos.y;
	SetMinMaxHeight( minHeight, maxHeight );
	hitbox.width = OTTO_WIDTH;
	hitbox.height = OTTO_HEIGHT;

	this->hDirection = ySpeed;
	this->vDirection = vDirection;
	this->dopefish = dopefish;

#ifdef _DEBUG
	shape.setFillColor( sf::Color::Transparent );
	shape.setOutlineColor( sf::Color::Red );
	shape.setOutlineThickness( 1.f );
	shape.setPosition( sf::Vector2f( hitbox.left, hitbox.top ) );
	shape.setSize( sf::Vector2f( hitbox.width, hitbox.height ) );
#endif
}

void EntityOtto::Think( const float dt )
{
	if( sprite.getTexture() == NULL )
	{
		sprite.setTexture( game->assetManager.GetTextureRef( "sprites" ) );
	}

    if (dopefish)
        sprite.setTextureRect( animManager.Animate( "dopefish" ) );
    else
        sprite.setTextureRect( animManager.Animate( "evil_otto" ) );

	if( hDirection == 0 ) hDirection = ySpeed; // Make sure this is initialized

	sf::Vector2f move = sf::Vector2f( 0, 0 );

	move.x = xSpeed * vDirection;

	if( ( hitbox.top + hitbox.height ) > maxHeight )
		hDirection = -ySpeed;
	else if( ( hitbox.top + hitbox.height ) < minHeight )
		hDirection = ySpeed;

	move.y += hDirection;

	Move( move, dt );
}

void EntityOtto::Draw() const
{
	this->game->window.draw( sprite );
#ifdef _DEBUG
	this->game->window.draw( shape );
#endif
}

void EntityOtto::HandleCollision( Entity *other )
{
	// Fool, nothing can stop Evil Otto!
}

void EntityOtto::SetMinMaxHeight( const float minHeight, const float maxHeight )
{
	this->maxHeight = maxHeight;
	this->minHeight = minHeight;
}

void EntityOtto::Move( sf::Vector2f move, const float dt ) // Add vector to produce movement
{
	hitbox.left += move.x * dt;
	hitbox.top += move.y * dt;
	sprite.setPosition( sf::Vector2f( hitbox.left, hitbox.top ) );
#ifdef _DEBUG
	shape.setPosition( sf::Vector2f( hitbox.left, hitbox.top ) );
#endif
}