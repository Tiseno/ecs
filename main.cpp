// Inspiration from https://www.david-colson.com/2020/02/09/making-a-simple-ecs.html
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <string_view>
#include <cstring>
#include <bitset>

#define foreground_reset     "\033[0m"
#define foreground_red       "\033[31m"
#define foreground_green     "\033[32m"
#define foreground_yellow    "\033[33m"
#define foreground_dark_blue "\033[34m"
#define foreground_magenta   "\033[35m"
#define foreground_cyan      "\033[36m"

#define foreground_underline       "\033[4m"

typedef unsigned int Tag;
typedef size_t EntityId;
typedef size_t ComponentId;

typedef double Z;

struct Vec3 { Z x, y, z; };
struct Color { Z r, g, b; };

struct Position { Vec3 pos; };
std::ostream &operator<<(std::ostream &os, Position const& m) { return os << foreground_yellow << "Position{" << m.pos.x << " " << m.pos.y << " " << m.pos.z << "}" << foreground_reset; }
struct Velocity { Vec3 vel; };
std::ostream &operator<<(std::ostream &os, Velocity const& m) { return os << foreground_yellow << "Velocity{" << m.vel.x << " " << m.vel.y << " " << m.vel.z << "}" << foreground_reset; }
struct Shape { Color color; };
// TODO make operators
struct Physical { Z mass; };
struct Size { Vec3 size; };
struct Anounce { char greeting[30]; };

ComponentId COMPONENT_ID = 0;
template<typename Component>
ComponentId id() {
	static ComponentId i = COMPONENT_ID++;
	return i;
}

template<typename Component>
Tag tag() {
	return 1 << id<Component>();
}

unsigned short MAX_ENTITIES = -1;

std::bitset<7> bits(Tag tag) {
	return std::bitset<7>(tag);
}

struct ComponentPool {
	ComponentId id;
	Tag tag;
	size_t componentSize;
	char * data;

	ComponentPool(ComponentId _id, Tag _tag, size_t _componentSize) :
		id(_id),
		tag(_tag),
		componentSize(_componentSize) {
		data = new char[componentSize * MAX_ENTITIES];
	}

	~ComponentPool() {
		delete[] data;
	}

	void* operator[](size_t i) {
		return data + i * componentSize;
	}
};

std::ostream &operator<<(std::ostream &os, ComponentPool const& m) { return os << foreground_green << "ComponentPool{" << m.id << " " << bits(m.tag) << "}" << foreground_reset; }

struct Entity {
	EntityId id;
	Tag components;
	static EntityId ENTITY_ID;

	// TODO ML reuse ids from deleted entities
	explicit Entity() : id(++ENTITY_ID), components() {}

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

std::ostream &operator<<(std::ostream &os, Entity const& m) { return os << foreground_magenta << "Entity{" << m.id << " " << bits(m.components) << "}" << foreground_reset; }

struct Components {
	std::vector<ComponentPool*> componentPools;

	Components() {}

	template<typename Component>
	Component* assign(Entity& entity) { // TODO pass component object as initializer
		if (componentPools.size() <= id<Component>()) {
			componentPools.resize(id<Component>() + 1, nullptr);
		}
		if (componentPools[id<Component>()] == nullptr) {
			ComponentPool* newPool = new ComponentPool(id<Component>(), tag<Component>(), sizeof(Component));
			componentPools[id<Component>()] = newPool;
			std::cout << "Created " << *newPool << "\n";
		}
		auto cp = new ((*componentPools[id<Component>()])[entity.id]) Component();
		entity.addComponent<Component>();
		std::cout << "Assigned component " << id<Component>() << " to " << entity << "\n";
		return cp;
	}

	template<typename Component>
	Component* get(EntityId entityId) {
		return (Component*)(*componentPools[id<Component>()])[entityId];
	}
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

std::ostream &operator<<(std::ostream &os, System const& m) { return os << foreground_cyan << "System{" << m.name << " " << bits(m.signature) << "}" << foreground_reset; }

struct AnouncePositionSystem : System {
	AnouncePositionSystem() : System("AnouncePositionSystem", tag<Anounce>() | tag<Position>()) {}

	void handleEntity(Entity & e, Components components) override {
		auto p = components.get<Position>(e.id);
		std::cout << "I am " << e << " at the position " << *p << "\n";
		// TODO how do we even store string data efficiently?
		// std::cout << "ANOUNCE MESSAGE " << (const char*)(components.get<Anounce>(e.id)) << "\n";
	}
};

struct MoveSystem : System {
	MoveSystem() : System("MoveSystem", tag<Position>() | tag<Velocity>()) {}

	void handleEntity(Entity & e, Components components) override {
		// TODO update the position of the entity using position and velocity
		std::cout << e << " updating position\n";
		components.get<Position>(e.id)->pos.x += components.get<Velocity>(e.id)->vel.x;
	}
};

struct CollisionSystem : System {
	CollisionSystem() : System("CollisionSystem", tag<Position>() | tag<Physical>() | tag<Size>()) {}

	void handleEntity(Entity & e, Components components) override {
		// TODO Update collision quadtree?
		// TODO create collision event?
		std::cout << e << " checking collisions\n";
	}
};

struct RenderSystem : System {
	RenderSystem() : System("RenderSystem", tag<Position>() | tag<Shape>()) {}

	void handleEntity(Entity & e, Components components) override {
		// TODO render to the scene with the shape and position of the entity
		std::cout << e << " rendering\n";
	}
};

typedef std::vector<Entity> EntityList;

struct Entities {
	EntityList entityList;

	Entities() {}

	// TODO Reuse and give the entity its ID here
	Entity& create() {
		entityList.push_back(Entity());
		std::cout << "Created " << entityList.back() << "\n";
		return entityList.back();
	}
};

struct Systems {
	std::vector<System*> systemList;

	void add(System* system) {
		systemList.push_back(system);
		std::cout << "Created " << *systemList.back() << "\n";
	}
};

int main() {
	Entities entities;
	Components components;

	Entity& ne = entities.create();
	components.assign<Anounce>(ne);

	Entity& ne2 = entities.create();
	components.assign<Anounce>(ne2);
	components.assign<Position>(ne2);
	components.assign<Velocity>(ne2)->vel = Vec3{0.1,0.1,0.2};
	components.assign<Shape>(ne2);

	Entity& ne3 = entities.create();
	components.assign<Anounce>(ne3);
	components.assign<Position>(ne3);

	Entity& ne4 = entities.create();
	components.assign<Position>(ne4);
	components.assign<Velocity>(ne4);
	components.assign<Shape>(ne4);
	components.assign<Physical>(ne4);
	components.assign<Size>(ne4);

	Systems systems;
	systems.add(new AnouncePositionSystem);
	systems.add(new MoveSystem);
	systems.add(new AnouncePositionSystem);
	systems.add(new RenderSystem);

	while (true) {
		std::cout << "#####################################\n\n";
		for (auto e : entities.entityList) {
			std::cout << e << "\n";
		}
		std::cout << "\n";
		for (System * s : systems.systemList) {
			std::cout << "System :: " << s->name << "\n";
			for (auto e : entities.entityList) {
				s->update(e, components);
			}
			std::cout << "\n";
		}
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}

