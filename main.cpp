#define DEBUG_1
#define DEBUG_2

// Heavy inspiration from https://www.david-colson.com/2020/02/09/making-a-simple-ecs.html
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <string_view>
#include <cstring>
#include <bitset>

#include "ansi_code.h"
#include "component.h"

std::string newStr = std::string(ANSI_FG_RED) + "new " + ANSI_RESET;
std::string assignStr = std::string(ANSI_FG_GRAY) + "assign " + ANSI_RESET;

typedef size_t ComponentId;
ComponentId COMPONENT_ID = 0;
template<typename Component>
ComponentId id() {
	static ComponentId i = COMPONENT_ID++;
	return i;
}

typedef unsigned int Tag;
template<typename Component>
Tag tag() {
	return 1 << id<Component>();
}

unsigned int MAX_ENTITIES = 100000;

std::bitset<9> bits(Tag tag) {
	return std::bitset<9>(tag);
}

struct ComponentPool {
	std::string name;
	Tag tag;
	size_t componentSize;
	size_t totalSize;
	char * data;

	ComponentPool(std::string const& _name, ComponentId _id, Tag _tag, size_t _componentSize) :
		name(_name),
		tag(_tag),
		componentSize(_componentSize),
		totalSize(componentSize * MAX_ENTITIES)
	{
		data = new char[totalSize];
	}

	~ComponentPool() {
		delete[] data;
	}

	void* operator[](size_t i) {
		return data + i * componentSize;
	}

private: // Disallow copying
	ComponentPool(ComponentPool const& old);
	ComponentPool& operator=(ComponentPool const& other);
};

std::ostream &operator<<(std::ostream &os, ComponentPool const& m) { return os << ANSI_FG_GREEN << "ComponentPool{" << m.name << " " << static_cast<float>(m.totalSize) / 1000000 << "MB " << bits(m.tag) << "}" << ANSI_RESET; }

typedef size_t EntityId;
struct Entity {
	EntityId id;
	Tag components;

	static EntityId ENTITY_ID;

	explicit Entity() : id(++ENTITY_ID), components() {}

	void deactivate() {
		components = 0;
	}

	template<typename Component>
	Entity& addComponent() {
		components = components | (tag<Component>());
		return *this;
	}

	template<typename Component>
	Entity& removeComponent() {
		components = components & (~(tag<Component>()));
		return *this;
	}
};

EntityId Entity::ENTITY_ID = 0;

std::ostream &operator<<(std::ostream &os, Entity const& m) { return os << ANSI_FG_MAGENTA << "Entity{" << m.id << " " << bits(m.components) << "}" << ANSI_RESET; }

struct Components {
	std::vector<ComponentPool*> componentPools;

	Components() {}

	template<typename Component>
	Component* assign(Entity& entity, Component const& init) {
		if (componentPools.size() <= id<Component>()) {
			componentPools.resize(id<Component>() + 1, nullptr);
		}
		if (componentPools[id<Component>()] == nullptr) {
			// TODO how can we template component pool? i.e. ComponentPool<Component>() instead of passing everything
			ComponentPool* newPool = new ComponentPool(Component::NAME, id<Component>(), tag<Component>(), sizeof(Component));
			componentPools[id<Component>()] = newPool;
#ifdef DEBUG_1
			std::cout << newStr << *newPool << "\n";
#endif
		}
		auto cp = new ((*componentPools[id<Component>()])[entity.id]) Component(init);
		entity.addComponent<Component>();
#ifdef DEBUG_2
		std::cout << assignStr << *cp << " to " << entity << "\n";
#endif
		return cp;
	}

	template<typename Component>
	Component* get(Entity const& entity) {
		if((entity.components & tag<Component>()) != tag<Component>()) {
			return nullptr;
		}
		return (Component*)(*componentPools[id<Component>()])[entity.id];
	}

	// template<typename Component>
	// Component* get(EntityId entityId) {
	// 	return (Component*)(*componentPools[id<Component>()])[entityId];
	// }
};

struct System {
	std::string name;
	Tag signature;

	System(std::string const& _name, Tag _signature) : name(_name), signature(_signature) {}

	bool matchesSignature(Tag components) {
		return (components & signature) == signature;
	}

	// TODO create iterator for all entities with this systems components
	void update(Entity & e, Components& components) {
		if (!matchesSignature(e.components)) {
			return;
		}
		handleEntity(e, components);
	}

	virtual void handleEntity(Entity & e, Components components) = 0;
};

std::ostream &operator<<(std::ostream &os, System const& m) { return os << ANSI_FG_CYAN << "System{" << m.name << " " << bits(m.signature) << "}" << ANSI_RESET; }

struct TrackPositionSystem : System {
	TrackPositionSystem() : System("TrackPositionSystem", tag<Position>()) {}

	void handleEntity(Entity & e, Components components) override {
		components.get<Position>(e);
	}
};

struct InspectSystem : System {
	InspectSystem() : System("InspectSystem", tag<Inspect>()) {}

	void handleEntity(Entity & e, Components components) override {
		std::cout << e << "\n";
		if(components.get<Type>(e) != nullptr)         { std::cout << "\t" << ANSI_FG_MAGENTA << "|" << ANSI_RESET << (*components.get<Type>(e))         << "\n"; }
		if(components.get<Position>(e) != nullptr)     { std::cout << "\t" << ANSI_FG_MAGENTA << "|" << ANSI_RESET << (*components.get<Position>(e))     << "\n"; }
		if(components.get<Velocity>(e) != nullptr)     { std::cout << "\t" << ANSI_FG_MAGENTA << "|" << ANSI_RESET << (*components.get<Velocity>(e))     << "\n"; }
		if(components.get<Acceleration>(e) != nullptr) { std::cout << "\t" << ANSI_FG_MAGENTA << "|" << ANSI_RESET << (*components.get<Acceleration>(e)) << "\n"; }
		if(components.get<Shape>(e) != nullptr)        { std::cout << "\t" << ANSI_FG_MAGENTA << "|" << ANSI_RESET << (*components.get<Shape>(e))        << "\n"; }
		if(components.get<Physical>(e) != nullptr)     { std::cout << "\t" << ANSI_FG_MAGENTA << "|" << ANSI_RESET << (*components.get<Physical>(e))     << "\n"; }
		if(components.get<Size>(e) != nullptr)         { std::cout << "\t" << ANSI_FG_MAGENTA << "|" << ANSI_RESET << (*components.get<Size>(e))         << "\n"; }
		if(components.get<Brain>(e) != nullptr)        { std::cout << "\t" << ANSI_FG_MAGENTA << "|" << ANSI_RESET << (*components.get<Brain>(e))        << "\n"; }
		if(components.get<Inspect>(e) != nullptr)      { std::cout << "\t" << ANSI_FG_MAGENTA << "|" << ANSI_RESET << (*components.get<Inspect>(e))      << "\n"; }
		std::cout << "\t" << ANSI_FG_MAGENTA << "*----------\n" << ANSI_RESET;
	}
};

struct MoveSystem : System {
	MoveSystem() : System("MoveSystem", tag<Position>() | tag<Velocity>()) {}

	void handleEntity(Entity & e, Components components/*, double deltaTime */) override {
		// TODO update the position of the entity using position and velocity
		components.get<Position>(e)->pos.x += components.get<Velocity>(e)->vel.x;
	}
};

struct CollisionSystem : System {
	CollisionSystem() : System("CollisionSystem", tag<Position>() | tag<Physical>() | tag<Size>()) {}

	void handleEntity(Entity & e, Components components) override {
		components.get<Position>(e);
		components.get<Physical>(e);
		components.get<Size>(e);
		// TODO Update collision quadtree?
		// TODO create collision event?
	}
};

struct RenderSystem : System {
	RenderSystem() : System("RenderSystem", tag<Position>() | tag<Shape>()) {}

	void handleEntity(Entity & e, Components components) override {
		components.get<Position>(e);
		components.get<Shape>(e);
		// TODO render to the scene with the shape and position of the entity
	}
};

typedef std::vector<Entity> EntityList;

struct Entities {
	EntityList entityList;

	Entities() {}

	// TODO Reuse and give the entity its ID here
	Entity& create() {
		entityList.push_back(Entity());
#ifdef DEBUG_2
		std::cout << newStr << entityList.back() << " there are now " << entityList.size() << "\n";
#endif
		return entityList.back();
	}
};

struct Systems {
	std::vector<System*> systemList;

	void add(System* system) {
		systemList.push_back(system);
#ifdef DEBUG_1
		std::cout << newStr << *systemList.back() << "\n";
#endif
	}
};


void createLord(Components& components, Entities& entities) {
		static size_t lordCounter = 0;
		Entity& ne = entities.create();
		components.assign(ne, Type("Lord", lordCounter++));
		components.assign(ne, Position{0,0,0});
		components.assign(ne, Size{10,10,10});
		components.assign(ne, Shape{});
		components.assign(ne, Velocity());
		components.assign(ne, Acceleration());
		components.assign(ne, Brain(0));
}

int main() {
	Systems systems;
	systems.add(new TrackPositionSystem);
	systems.add(new MoveSystem);
	systems.add(new CollisionSystem);
	systems.add(new TrackPositionSystem);
	systems.add(new RenderSystem);
	systems.add(new InspectSystem);

	Entities entities;
	Components components;
	{
		Entity& ne = entities.create();
		components.assign(ne, Type("Narmud", 1));
		components.assign(ne, Position());

		Entity& ne2 = entities.create();
		components.assign(ne2, Type("Dumran", 1));
		components.assign(ne2, Position());
		components.assign(ne2, Velocity{0.1,0.1,0.2});
		components.assign(ne2, Shape{0,0,0});

		Entity& ne3 = entities.create();
		components.assign(ne3, Position());

		Entity& ne4 = entities.create();
		components.assign(ne4, Position());
		components.assign(ne4, Velocity());
		components.assign(ne4, Shape());
		components.assign(ne4, Physical());
		components.assign(ne4, Size());
		components.assign(ne4, Brain{50});
	}

	while (true) {
#ifdef DEBUG_2
		std::cout << "\n#####################################\n\n";
#endif
		components.get<Brain>(entities.entityList[3])->increase(0.1);
		createLord(components, entities);
		for (System * s : systems.systemList) {
			for (auto e : entities.entityList) {
				s->update(e, components);
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(400));
	}
}

