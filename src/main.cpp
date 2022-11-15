#include "raylib.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>

using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::system_clock;

struct sBall
{
	float px, py;
	float vx, vy;
	float ax, ay;
	float radius;
	float mass;

	int id;
};

std::vector<std::pair<float, float>> circleModel;
std::vector<sBall> vecBalls;
sBall* pSelectedBall = nullptr;

void AddBall(float x, float y, float r = 5.0f)
{
	sBall b;
	b.px = x;
	b.py = y;
	b.vx = 0;
	b.vy = 0;
	b.ax = 0;
	b.ay = 0;
	b.radius = r;
	b.mass = r * 100.0f;

	b.id = vecBalls.size();
	vecBalls.emplace_back(b);
}

int main()
{
	const int screenWidth = 800;
	const int screenHeight = 450;

	InitWindow(screenWidth, screenHeight, "Balls");

	SetTargetFPS(170);

	// Ball stuff
	circleModel.push_back({ 0.0f, 0.0f });
	//int nPoints = 20;
	//for (int i = 0; i < nPoints; i++)
	//	circleModel.push_back({cosf(i / (float)(nPoints) * 2.0f * 3.14159f), sinf(i / (float)(nPoints - 1) * 2.0f * 3.14159f)});

	float fDefaultRadius = 20.0f;
//	AddBall(screenWidth * 0.25f, screenHeight * 0.5f, fDefaultRadius);
//	AddBall(screenWidth * 0.75f, screenHeight * 0.5f, fDefaultRadius);

	for (int i = 0; i < 20; ++i)
	{
		AddBall(rand() % screenWidth, rand() % screenHeight, rand() % 32 + 10);
	}

	double deltaTime;
	double oldTime;

	while (!WindowShouldClose())
	{
		auto milsec = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
		deltaTime = milsec - oldTime;
		oldTime = milsec;
		double fps = (1.0 / deltaTime) * 1000;

		auto DoCirclesOverlap = [](float x1, float y1, float r1, float x2, float y2, float r2)
		{
			return fabs((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2)) <= (r1 + r2) * (r1 + r2);
		};

		auto IsPointInCircle = [](float x1, float y1, float r1, float px, float py)
		{
			return fabs((x1 - px) * (x1 - px) + (y1 - py) * (y1 - py)) < (r1 * r1);
		};

		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
		{
			pSelectedBall = nullptr;
			for (auto& ball: vecBalls)
			{
				if (IsPointInCircle(ball.px, ball.py, ball.radius, GetMousePosition().x, GetMousePosition().y))
				{
					pSelectedBall = &ball;
					break;
				}
			}
		}

		if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
		{
			pSelectedBall = nullptr;
		}

		if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT))
		{
			if (pSelectedBall != nullptr)
			{
				// Velocity
				pSelectedBall->vx = 0.02f * ((pSelectedBall->px) - (float)GetMousePosition().x);
				pSelectedBall->vy = 0.02f * ((pSelectedBall->py) - (float)GetMousePosition().y);

				pSelectedBall = nullptr;
			}
		}

		if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
		{
			if (pSelectedBall != nullptr)
			{
				pSelectedBall->px = GetMousePosition().x;
				pSelectedBall->py = GetMousePosition().y;
			}
		}

		std::vector<std::pair<sBall*, sBall*>> vecCollidingPairs;

		// Magic velocity
		for (auto& ball: vecBalls)
		{
			ball.vx = ball.vx * 0.99f;
			ball.vy = ball.vy * 0.99f;
			ball.vx += ball.ax * deltaTime;
			ball.vy += ball.ay * deltaTime;
			ball.px += ball.vx * deltaTime;
			ball.py += ball.vy * deltaTime;

			if (fabs(ball.vx * ball.vx + ball.vy * ball.vy) < 0.001f)
			{
				ball.vx = 0;
				ball.vy = 0;
			}

			// Wrap the balls around screen
			if (ball.px < 0) ball.px += (float)screenWidth;
			if (ball.px >= screenWidth) ball.px -= (float)screenWidth;
			if (ball.py < 0) ball.py += (float)screenHeight;
			if (ball.py >= screenHeight) ball.py -= (float)screenHeight;
		}

		for (auto& ball: vecBalls)
		{
			for (auto& target: vecBalls)
			{
				if (ball.id != target.id)
				{
					if (DoCirclesOverlap(ball.px, ball.py, ball.radius, target.px, target.py, target.radius))
					{
						// Collide
						vecCollidingPairs.push_back({ &ball, &target });

						// Balls distance
						float fDistance = sqrtf((ball.px - target.px) * (ball.px - target.px)
												+ (ball.py - target.py) * (ball.py - target.py));

						float fOverlap = 0.5f * (fDistance - ball.radius - target.radius);

						// Move Ball one
						ball.px -= fOverlap * (ball.px - target.px) / fDistance;
						ball.py -= fOverlap * (ball.py - target.py) / fDistance;
						// Move Ball two
						target.px += fOverlap * (ball.px - target.px) / fDistance;
						target.py += fOverlap * (ball.py - target.py) / fDistance;
					}
				}
			}
		}

		// Dynamic collision
		for (auto c: vecCollidingPairs)
		{
			sBall* b1 = c.first;
			sBall* b2 = c.second;

			// Ball distance
			float fDistance = sqrtf((b1->px - b2->px) * (b1->px - b2->px) + (b1->py - b2->py) * (b1->py - b2->py));

			// Normals
			float nx = (b2->px - b1->px) / fDistance;
			float ny = (b2->py - b1->py) / fDistance;
			float dNorm1 = b1->vx * nx + b1->vy * ny;
			float dNorm2 = b2->vx * nx + b2->vy * ny;

			// Tangents
			float tx = -ny;
			float ty = nx;
			float dtan1 = b1->vx * tx + b1->vy * ty;
			float dtan2 = b2->vx * tx + b2->vy * ty;

			// Conservation of momentum in 1D
			float m1 = (dNorm1 * (b1->mass - b2->mass) + 2.0f * b2->mass * dNorm2) / (b1->mass + b2->mass);
			float m2 = (dNorm2 * (b2->mass - b1->mass) + 2.0f * b1->mass * dNorm1) / (b1->mass + b2->mass);

			b1->vx = tx * dtan1 + nx * m1;
			b1->vy = ty * dtan1 + ny * m1;
			b2->vx = tx * dtan2 + nx * m2;
			b2->vy = ty * dtan2 + ny * m2;
		}

		BeginDrawing();

		ClearBackground(RAYWHITE);

		for (const auto& ball: vecBalls)
		{
			DrawCircleLines(ball.px, ball.py, ball.radius, BLACK);
		}

		for (auto c: vecCollidingPairs)
		{
			DrawLine(c.first->px, c.first->py, c.second->px, c.second->py, RED);
		}

		if (pSelectedBall != nullptr)
		{
			DrawLine(pSelectedBall->px, pSelectedBall->py, GetMousePosition().x, GetMousePosition().y, GREEN);
		}

		DrawText(std::to_string(fps).c_str(), 20, 20, 8, BLACK);

		EndDrawing();
	}
	CloseWindow();

	return 0;
}