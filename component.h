#pragma once
#include <iostream>
#include <cstring>

#include "ansi_code.h"

typedef double Z;

struct Vec3 { Z x, y, z; };
struct Color { uint8_t r, g, b; };

struct Position {
	Vec3 pos;
	static std::string NAME;
};
std::ostream &operator<<(std::ostream &os, Position const& m) { return os << ANSI_FG_YELLOW << "Position{" << m.pos.x << " " << m.pos.y << " " << m.pos.z << "}" << ANSI_RESET; }
std::string Position::NAME = "Position";

struct Velocity {
	Vec3 vel;
	static std::string NAME;
};
std::ostream &operator<<(std::ostream &os, Velocity const& m) { return os << ANSI_FG_YELLOW << "Velocity{" << m.vel.x << " " << m.vel.y << " " << m.vel.z << "}" << ANSI_RESET; }
std::string Velocity::NAME = "Velocity";

struct Acceleration {
	Vec3 acc;
	static std::string NAME;
};
std::ostream &operator<<(std::ostream &os, Acceleration const& m) { return os << ANSI_FG_YELLOW << "Acceleration{" << m.acc.x << " " << m.acc.y << " " << m.acc.z << "}" << ANSI_RESET; }
std::string Acceleration::NAME = "Acceleration";

struct Shape {
	Color color;
	static std::string NAME;
};
std::ostream &operator<<(std::ostream &os, Shape const& m) { return os << ANSI_FG_YELLOW << "Shape{" << (int)m.color.r << " " << (int)m.color.g << " " << (int)m.color.b << "}" << ANSI_RESET; }
std::string Shape::NAME = "Shape";

struct Physical {
	Z mass;
	static std::string NAME;
};
std::ostream &operator<<(std::ostream &os, Physical const& m) { return os << ANSI_FG_YELLOW << "Physical{" << m.mass << "}" << ANSI_RESET; }
std::string Physical::NAME = "Physical";

struct Size {
	Vec3 size;
	std::string static NAME;
};
std::ostream &operator<<(std::ostream &os, Size const& m) { return os << ANSI_FG_YELLOW << "Size{" << m.size.x << " " << m.size.y << " " << m.size.z << "}" << ANSI_RESET; }
std::string Size::NAME = "Size";

struct Brain {
private:
	Z m_iq;
public:
	static const Z MAX_IQ;
	explicit Brain(Z _iq) : m_iq(0) {
		increase(_iq);
	}
	inline Z iq() const { return m_iq; }
	inline void increase(Z _iq) {
		m_iq += _iq;
		if(m_iq < -MAX_IQ)
			m_iq = -MAX_IQ;
		if(m_iq > MAX_IQ)
			m_iq = MAX_IQ;
	}
	static std::string NAME;
};
std::ostream &operator<<(std::ostream &os, Brain const& m) { return os << ANSI_FG_YELLOW << "Brain{" << m.iq() << "}" << ANSI_RESET; }
std::string Brain::NAME = "Brain";
const Z Brain::MAX_IQ = 1 << 8;

struct Inspect {
	bool _;
	static std::string NAME;
};
std::ostream &operator<<(std::ostream &os, Inspect const& m) { return os << ANSI_FG_YELLOW << "Inspect{" << m._ << "}" << ANSI_RESET; }
std::string Inspect::NAME = "Inspect";

#define STR_MAX 16
struct Type {
	// TODO maybe replace this with an enum instead
	// TypeId typeId;
	char name[STR_MAX];
	size_t id;

	explicit Type(std::string const& _name, size_t _id) : id(_id) {
		strcpy(name, _name.substr(0,STR_MAX-1).c_str());
	}

	// const char* typeName() const {
	// 	return m_typeName;
	// }

	// TODO this is how we should handle mutable strings
	// Type& set(std::string s) {
	// 	strcpy(_type, s.substr(0,STR_MAX).c_str());
	// 	return *this;
	// }
	static std::string NAME;
};
std::ostream &operator<<(std::ostream &os, Type const& m) { return os << ANSI_FG_YELLOW << "Type{" << m.name << " " << m.id << "}" << ANSI_RESET; }
std::string Type::NAME = "Type";

