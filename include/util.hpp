#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include "Vector4.hpp"
#include <math.h>
#include <bitset>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <iterator>
#include <list>
#include <algorithm>


const double PI = 3.141592653589793238463;
const float  PI_F = 3.14159265358979f;

struct fps_counter {
public:
	fps_counter() : 
		backdrop(sf::Vector2f(200, 100)), vertex_array(sf::LinesStrip) {
		
		backdrop.setFillColor(sf::Color(0x0000003F));

		if(!f.loadFromFile("../assets/fonts/Arial.ttf")){
				std::cout << "couldn't find the fall back Arial font in ../assets/fonts/" << std::endl;
		} else {
			t.setFont(f);
			t.setCharacterSize(18);
			t.setColor(sf::Color::White);
		}
	}

	void frame(double delta_time){
    // Apply 100 units of smoothing
		if (frame_count == 100){
			frame_count = 0;
			fps_average = 0;
		}
			frame_count++;
			fps_average += (delta_time - fps_average) / frame_count;
	}

	void flip_units() {
		if (milliseconds)
			milliseconds = false;
		else
			milliseconds = true;
	}

	void draw(sf::RenderWindow *r){
		
		r->draw(backdrop);
		
		if (vertex_position == 200)
			vertex_position = 0;

		sf::Vector2f origin = backdrop.getPosition();
		sf::Vector2f point = origin + sf::Vector2f(vertex_position, backdrop.getSize().y - (1.0/fps_average));

		if (vertex_array.getVertexCount() < 200)
			vertex_array.append(sf::Vertex(point, sf::Color::Red));
		else
			vertex_array[vertex_position] = sf::Vertex(point, sf::Color::Red);

		r->draw(vertex_array);

		vertex_position++;

		std::string out;

		if (milliseconds)
			out = std::to_string(fps_average);
		else
			out = std::to_string(floor(1 / fps_average));

		t.setString(out);



		r->draw(t);
	}

private:

	sf::RectangleShape backdrop;
	sf::VertexArray vertex_array;
	
	sf::Font f;
	sf::Text t;

	int frame_count = 0;
	double fps_average = 0;
	bool milliseconds = false;

	int vertex_position = 0;
};


inline sf::Vector3f SphereToCart(sf::Vector2f i) {

	auto r = sf::Vector3f(
		(1 * sin(i.y) * cos(i.x)),
		(1 * sin(i.y) * sin(i.x)),
		(1 * cos(i.y))
		);
	return r;
};

inline sf::Vector3f SphereToCart(sf::Vector3f i) {

	auto r = sf::Vector3f(
		(i.x * sin(i.z) * cos(i.y)),
		(i.x * sin(i.z) * sin(i.y)),
		(i.x * cos(i.z))
		);
	return r;
};


inline sf::Vector3f CartToSphere(sf::Vector3f in) {

	auto r = sf::Vector3f(
		sqrt(in.x * in.x + in.y * in.y + in.z * in.z),
		atan(in.y / in.x),
		atan(sqrt(in.x * in.x + in.y * in.y) / in.z)
		);
	return r;
};

inline sf::Vector2f CartToNormalizedSphere(sf::Vector3f in) {

	auto r = sf::Vector2f(
		atan2(sqrt(in.x * in.x + in.y * in.y), in.z),
		atan2(in.y, in.x)
		);

	return r;
}

inline sf::Vector3f FixOrigin(sf::Vector3f base, sf::Vector3f head) {
	return head - base;
}

inline sf::Vector3f Normalize(sf::Vector3f in) {

	float multiplier = sqrt(in.x * in.x + in.y * in.y + in.z * in.z);
	auto r = sf::Vector3f(
		in.x / multiplier,
		in.y / multiplier,
		in.z / multiplier
	);
	return r;

}

inline float DotProduct(sf::Vector3f a, sf::Vector3f b){
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline float Magnitude(sf::Vector3f in){
	return sqrt(in.x * in.x + in.y * in.y + in.z * in.z);
}

inline float AngleBetweenVectors(sf::Vector3f a, sf::Vector3f b){
	return acos(DotProduct(a, b) / (Magnitude(a) * Magnitude(b)));
}

inline float DistanceBetweenPoints(sf::Vector3f a, sf::Vector3f b) {
	return sqrt(DotProduct(a, b));
}

inline float DegreesToRadians(float in) {
	return static_cast<float>(in * PI / 180.0f);
}

inline float RadiansToDegrees(float in) {
	return static_cast<float>(in * 180.0f / PI);
}

inline std::string read_file(std::string file_name){
	std::ifstream input_file(file_name);

	if (!input_file.is_open()){
		std::cout << file_name << " could not be opened" << std::endl;
		return "";
	}

	std::stringstream buf;
	buf << input_file.rdbuf();
	input_file.close();
	return buf.str();
}

inline void PrettyPrintUINT64(uint64_t i, std::stringstream* ss) {

	*ss << "[" << std::bitset<15>(i) << "]";
	*ss << "[" << std::bitset<1>(i >> 15) << "]";
	*ss << "[" << std::bitset<8>(i >> 16) << "]";
	*ss << "[" << std::bitset<8>(i >> 24) << "]";
	*ss << "[" << std::bitset<32>(i >> 32) << "]";

}

inline void PrettyPrintUINT64(uint64_t i) {

	std::cout << "[" << std::bitset<15>(i) << "]";
	std::cout << "[" << std::bitset<1>(i >> 15) << "]";
	std::cout << "[" << std::bitset<8>(i >> 16) << "]";
	std::cout << "[" << std::bitset<8>(i >> 24) << "]";
	std::cout << "[" << std::bitset<32>(i >> 32) << "]" << std::endl;

}

inline void DumpLog(std::stringstream* ss, std::string file_name) {

	std::ofstream log_file;
	log_file.open(file_name);

	log_file << ss->str();

	log_file.close();

}

inline int count_bits(int32_t v) {

	v = v - ((v >> 1) & 0x55555555);                       // reuse input as temporary
	v = (v & 0x33333333) + ((v >> 2) & 0x33333333);        // temp
	return (((v + (v >> 4)) & 0xF0F0F0F) * 0x1010101) >> 24; // count
}

inline int count_bits(int64_t v) {

	int32_t left = (int32_t)(v);
	int32_t right = (int32_t)(v >> 32);

	left = left - ((left >> 1) & 0x55555555);                    // reuse input as temporary
	left = (left & 0x33333333) + ((left >> 2) & 0x33333333);     // temp
	left = ((left + (left >> 4) & 0xF0F0F0F) * 0x1010101) >> 24; // count

	right = right - ((right >> 1) & 0x55555555);                    // reuse input as temporary
	right = (right & 0x33333333) + ((right >> 2) & 0x33333333);     // temp
	right = ((right + (right >> 4) & 0xF0F0F0F) * 0x1010101) >> 24; // count

	return left + right;
}
