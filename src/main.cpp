
#include <iostream>
#include <SFML/Graphics.hpp>
#include <random>
#include <chrono>
#include "util.hpp"
#include <thread>
#include "OpenCL.h"

float elap_time() {
	static std::chrono::time_point<std::chrono::system_clock> start;
	static bool started = false;

	if (!started) {
		start = std::chrono::system_clock::now();
		started = true;
	}

	std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed_time = now - start;
	return static_cast<float>(elapsed_time.count());
}

const int WINDOW_X = 1920;
const int WINDOW_Y = 1080;

float scale(float valueIn, float origMin, float origMax, float scaledMin, float scaledMax) {
	return ((scaledMax - scaledMin) * (valueIn - origMin) / (origMax - origMin)) + scaledMin;
}

void func(int id, int count, sf::Uint8* pixels) {

	for (int pixel_x = 0; pixel_x < WINDOW_X; pixel_x++) {

		for (int pixel_y = (WINDOW_Y * ((float)id / count)); pixel_y < (WINDOW_Y * ((float)(id + 1) / count)); pixel_y++) {

			float y0 = scale(pixel_y, 0, WINDOW_Y, -1.0f, 1.0f);
			float x0 = scale(pixel_x, 0, WINDOW_X, -2.0f, 1.0f);

			float x = 0.0;
			float y = 0.0;

			int iteration_count = 0;
			int interation_threshold = 1000;

			while (x*x + y*y < 4 && iteration_count < interation_threshold) {
				float x_temp = x*x - y*y + x0;
				y = 2 * x * y + y0;
				x = x_temp;
				iteration_count++;
			}

			sf::Color c(0, 0, scale(iteration_count, 0, 1000, 0, 255), 255);
			int val = scale(iteration_count, 0, 1000, 0, 16777216);

			pixels[(pixel_y * WINDOW_X + pixel_x) * 4 + 0] = val & 0xff;
			pixels[(pixel_y * WINDOW_X + pixel_x) * 4 + 1] = (val >> 8) & 0xff;
			pixels[(pixel_y * WINDOW_X + pixel_x) * 4 + 2] = (val >> 16) & 0xff;
			pixels[(pixel_y * WINDOW_X + pixel_x) * 4 + 3] = 200;
		}
	}
}

enum Mouse_State {PRESSED, DEPRESSED};

int main() {
	
	std::mt19937 rng(time(NULL));
	std::uniform_int_distribution<int> rgen(100, 400);

	sf::RenderWindow window(sf::VideoMode(WINDOW_X, WINDOW_Y), "quick-sfml-template");
	window.setFramerateLimit(60);

	float physic_step = 0.166f;
	float physic_time = 0.0f;

	double frame_time = 0.0, elapsed_time = 0.0, delta_time = 0.0, accumulator_time = 0.0, current_time = 0.0;
	fps_counter fps;
	
	OpenCL cl(sf::Vector2i(WINDOW_X, WINDOW_Y));

	sf::Vector4f range(-1.0f, 1.0f, -1.0f, 1.0f);
	cl.init(&range);


	while (window.isOpen())
	{
		sf::Event event; // Handle input
		while (window.pollEvent(event)) {
			if (event.type == sf::Event::Closed) {
				window.close();
			}
			if (event.type == sf::Event::KeyPressed) {
				if (event.key.code == sf::Keyboard::Down) {
					range.z += 0.001;
					range.w += 0.001;
				}
				if (event.key.code == sf::Keyboard::Up) {
					range.z -= 0.001;
					range.w -= 0.001;
				}
				if (event.key.code == sf::Keyboard::Right) {
					range.x += 0.001;
					range.y += 0.001;
				}
				if (event.key.code == sf::Keyboard::Left) {
					range.x -= 0.001;
					range.y -= 0.001;
				}
				if (event.key.code == sf::Keyboard::Equal) {
					range.x *= 1.02;
					range.y *= 1.02;
					range.z *= 1.02;
					range.w *= 1.02;
				}
				if (event.key.code == sf::Keyboard::Dash) {
					range.x *= 0.98;
					range.y *= 0.98;
					range.z *= 0.98;
					range.w *= 0.98;
				}
			}
		}

		elapsed_time = elap_time(); // Handle time
		delta_time = elapsed_time - current_time;
		current_time = elapsed_time;
		if (delta_time > 0.02f)
			delta_time = 0.02f;
		accumulator_time += delta_time;

		while (accumulator_time >= physic_step) { // While the frame has sim time, update 
		
			accumulator_time -= physic_step;
			physic_time += physic_step;

			// Do physics at 60fps
		}

		cl.run_kernel("mandlebrot");

		window.clear(sf::Color::White);

		cl.draw(&window);

		//window.draw(viewport_sprite);

		fps.draw(&window);
		fps.frame(delta_time);

		window.display();

	}
	return 0;

}
