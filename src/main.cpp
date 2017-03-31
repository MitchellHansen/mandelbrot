
#include <iostream>
#include <SFML/Graphics.hpp>
#include <random>
#include <chrono>
#include "util.hpp"
#include <thread>
#include <vector>

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

			while (pow(x, 2) + pow(y, 2) < pow(2, 2) && iteration_count < interation_threshold) {
				float x_temp = pow(x, 2) - pow(y, 2) + x0;
				y = 2 * x * y + y0;
				x = x_temp;
				iteration_count++;
			}

			sf::Color c(0, 0, scale(iteration_count, 0, 1000, 0, 255), 255);
			int val = scale(iteration_count, 0, 1000, 0, pow(2, 24));

			pixels[(pixel_y * WINDOW_X + pixel_x) * 4 + 0] = val & 0xff;
			pixels[(pixel_y * WINDOW_X + pixel_x) * 4 + 1] = (val >> 8) & 0xff;
			pixels[(pixel_y * WINDOW_X + pixel_x) * 4 + 2] = (val >> 16) & 0xff;
			pixels[(pixel_y * WINDOW_X + pixel_x) * 4 + 3] = 200;
			//memcpy(&pixels[(pixel_y * pixel_x + pixel_y) * 4], (void*)&c, sizeof(c));
		}
	}
}

int main() {
	
	std::mt19937 rng(time(NULL));
	std::uniform_int_distribution<int> rgen(100, 400);

	sf::RenderWindow window(sf::VideoMode(WINDOW_X, WINDOW_Y), "quick-sfml-template");
	window.setFramerateLimit(60);


	float physic_step = 0.166f;
	float physic_time = 0.0f;

	double frame_time = 0.0, elapsed_time = 0.0, delta_time = 0.0, accumulator_time = 0.0, current_time = 0.0;
	fps_counter fps;
	
	sf::Uint8 *pixels = new sf::Uint8[WINDOW_X * WINDOW_Y * 4];

	sf::Sprite viewport_sprite;
	sf::Texture viewport_texture;
	viewport_texture.create(WINDOW_X, WINDOW_Y);
	viewport_texture.update(pixels);
	viewport_sprite.setTexture(viewport_texture);


	std::vector<std::thread> thread_pool;

	for (int i = 0; i < 10; i++) {
		thread_pool.emplace_back(std::thread(func, i, 10, pixels));
	}

	for (auto &t: thread_pool) {
		t.join();
	}

	viewport_texture.update(pixels);

	while (window.isOpen())
	{
		sf::Event event; // Handle input
		while (window.pollEvent(event)) {
			if (event.type == sf::Event::Closed) {
				window.close();
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

		window.clear(sf::Color::White);

		window.draw(viewport_sprite);

		fps.draw(&window);
		fps.frame(delta_time);

		window.display();

	}
	return 0;

}