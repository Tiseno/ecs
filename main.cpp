#include <iostream>
#include <vector>
#include <thread>
#include <chrono>

typedef unsigned int Tag;
typedef size_t EntityId;

// TODO add component data such as a greeting component containing the entities greeting

EntityId ID = 10000;

const Tag POSITION_COMPONENT = 1 << 1;
const Tag VELOCITY_COMPONENT = 1 << 2;
const Tag SHAPE_COMPONENT    = 1 << 3;
const Tag SOLID_COMPONENT    = 1 << 4;
const Tag SIZE_COMPONENT     = 1 << 5;
const Tag COMPONENT_7        = 1 << 6;
const Tag COMPONENT_8        = 1 << 7;
const Tag ANOUNCE_POSITION_COMPONENT  = 1 << 0;

struct Entity {
	Tag components;
	EntityId id;
	Entity(Tag _components) : id(++ID), components(_components) {}
};

typedef std::vector<Entity> EntityList;

struct System {
	Tag signature;
	std::string name;

	System(std::string _name, Tag _signature) : name(_name), signature(_signature) {
		std::cout << "Created a" << name << "\n";
	}

	bool matchesSignature(Tag components) {
		return (components & signature) == signature;
	}

	void update(Entity & e) {
		if (!matchesSignature(e.components)) {
			return;
		}
		handleEntity(e);
	}

	virtual void handleEntity(Entity & e) {

	}
};

struct AnouncePositionSystem : System {
	AnouncePositionSystem() : System("AnouncePositionSystem", ANOUNCE_POSITION_COMPONENT | POSITION_COMPONENT) {}

	virtual void handleEntity(Entity & e) {
		// TODO get position
		std::cout << "I am " << e.id << " at the position 0,0\n";
	}
};

struct MoveSystem : System {
	MoveSystem() : System("MoveSystem", POSITION_COMPONENT | VELOCITY_COMPONENT) {}

	virtual void handleEntity(Entity & e) {
		// TODO update the position of the entity using position and velocity
		std::cout << "Updating position of " << e.id << "\n";
	}
};

struct CollisionSystem : System {
	CollisionSystem() : System("CollisionSystem", POSITION_COMPONENT | SOLID_COMPONENT | SIZE_COMPONENT) {}

	virtual void handleEntity(Entity & e) {
		// TODO Update collision quadtree?
		// TODO create collision event?
		std::cout << "Checking for collisions of " << e.id << "\n";
	}
};

struct RenderSystem : System {
	RenderSystem() : System("RenderSystem", POSITION_COMPONENT | SHAPE_COMPONENT) {}

	virtual void handleEntity(Entity & e) {
		// TODO render to the scene with the shape and position of the entity
		std::cout << "Rendering " << e.id << "\n";
	}
};

struct Entities {
	EntityList entityList;

	Entities() {}

	void addEntity(Entity e) {
		std::cout << "Adding entity " << e.id << "\n";
		entityList.push_back(e);
		std::cout << "There are now " << entityList.size() << " entities!" << "\n";
	}
};

int main() {
	Entities entities;
	entities.addEntity(Entity(ANOUNCE_POSITION_COMPONENT));
	entities.addEntity(Entity(ANOUNCE_POSITION_COMPONENT | POSITION_COMPONENT | VELOCITY_COMPONENT | SHAPE_COMPONENT));
	entities.addEntity(Entity(ANOUNCE_POSITION_COMPONENT | POSITION_COMPONENT));

	std::vector<System*> systems;
	systems.push_back(new AnouncePositionSystem);
	systems.push_back(new MoveSystem);
	systems.push_back(new AnouncePositionSystem);
	systems.push_back(new RenderSystem);

	while (true) {
		std::cout << "#####################################\n\n";
		for (System * s : systems) {
			std::cout << "System : " << s->name << "\n";
			for (auto e : entities.entityList) {
				s->update(e);
			}
			std::cout << "\n";
		}
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}

