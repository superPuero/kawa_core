#include "../kawa/core/core.h"
#include "raylib/include/raylib.h"


#define $win_h 800
#define $win_w 800

#define $x_ext 10
#define $y_ext 10

using namespace kawa;

struct data
{
	u8 temperature;
	u8 step;
};

int main()
{

	registry entities({ .max_entity_count = 0xFFF });

	for (usize y = 0; y < $y_ext; y++)
	{
		for (usize x = 0; x < $x_ext; x++)
		{				
			entities.entity(
				Vector2{ (float)($win_w / $x_ext) * x, (float)($win_h / $y_ext) * y },
				data{ (u8)((255 / $x_ext) * x) }
			);
		}
	}

	InitWindow($win_w, $win_h, "foo");

	while(!WindowShouldClose())
	{
		entities.query(
			[&](const Vector2& vec, data& d)
			{
				entities.query(
					[&](const Vector2& vec2, data& d2)
					{
						if(vec.x )
					}
				);
			}
		);

		BeginDrawing();

		entities.query(
			[](const Vector2& vec, const Color& col)
			{
				DrawRectangle(vec.x, vec.y, $win_w / $x_ext, $win_h / $y_ext, col);
			}
		);

		EndDrawing();

		PollInputEvents();
	}
}