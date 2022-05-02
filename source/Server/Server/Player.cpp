﻿#include "pch.hpp"
#include "Player.hpp"

CPlayer::CPlayer()
{
	name[0] = 0;
}

CPlayer::CPlayer(short x, short y, short z) : x(x), y(y), z(z)
{
	name[0] = 0;
}

CPlayer::CPlayer(char* new_name)
{
	strcpy_s(name, new_name);
}

CPlayer::CPlayer(short x, short y, short z, char* new_name) : x(x), y(y), z(z)
{
	strcpy_s(name, new_name);
}

void CPlayer::Move(int direction)
{
	int input_count{ 0 };

	if (direction & DIRECTION::FORWARD)
	{
		++input_count;
	}
	if (direction & DIRECTION::BACKWARD)
	{
		--input_count;
	}
	if (direction & DIRECTION::RIGHT)
	{
		++input_count;
	}
	if (direction & DIRECTION::LEFT)
	{
		--input_count;
	}

	switch (direction)
	{
	case DIRECTION::FORWARD:
	{
		z += 10.0f;
	}
	break;
	case DIRECTION::BACKWARD:
	{
		z -= 10.0f;
	}
	break;
	case DIRECTION::LEFT:
	{
		x += 10.0f;
	}
	break;
	case DIRECTION::RIGHT:
	{
		x -= 10.0f;
	}
	break;
	case DIRECTION::UP:
	{
		y += 10.0f;
	}
	break;
	case DIRECTION::DOWN:
	{
		y -= 10.0f;
	}
	break;
	default:
		break;
	}

	std::cout << x << ", " << y << ", " << z << std::endl;
}
