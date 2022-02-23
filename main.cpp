// Inspiration from https://www.david-colson.com/2020/02/09/making-a-simple-ecs.html
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <string_view>
#include <cstring>

typedef unsigned int Tag;
typedef size_t EntityId;
typedef size_t ComponentId;

EntityId ENTITY_ID = 0;

typedef double Z;

struct Vec3 { Z x, y, z; };
struct Color { Z r, g, b; };

struct Position { Vec3 pos; };
struct Velocity { Vec3 vel; };
struct Shape { Color color; };
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

struct ComponentPool {
	Tag tag;
	size_t componentSize;
	char * data;

	ComponentPool(Tag _tag, size_t _componentSize) : tag(_tag), componentSize(_componentSize) {
		std::cout << "Creating component pool for tag " << tag << "\n";
		std::cout << "With max entities " << (MAX_ENTITIES) << "\n";
		std::cout << "And resulting size " << (componentSize * MAX_ENTITIES) << "\n";
		data = new char[componentSize * MAX_ENTITIES];
	}

	~ComponentPool() {
		delete[] data;
	}

	void* operator[](size_t i) {
		return data + i * componentSize;
	}
};

struct Entity {
	EntityId id;
	Tag components;

	// TODO ML reuse ids from deleted entities
	explicit Entity() : id(++ENTITY_ID), components() {}

	template<typename Component>
	Entity& addComponent() {
		std::cout << "add component to entity with component tag\n";
		std::cout << "before mutation: " << components << "\n";
		components = components | (tag<Component>());
		std::cout << "after mutation:  " << components << "\n";
		return *this;
	}

	template<typename Component>
	Entity& removeComponent() {
		components = components & (~(tag<Component>()));
		return *this; }
};

struct Components {
	std::vector<ComponentPool*> componentPools;

	Components() {}

	template<typename Component>
	Component* assign(Entity& entity) { // TODO pass component object as initializer
		std::cout << "Assigning component " << id<Component>() << " to " << entity.id << "\n";
		if (componentPools.size() <= id<Component>()) {
			componentPools.resize(id<Component>() + 1, nullptr);
		}
		if (componentPools[id<Component>()] == nullptr) {
			componentPools[id<Component>()] = new ComponentPool(tag<Component>(), sizeof(Component));
		}
		auto cp = new ((*componentPools[id<Component>()])[entity.id]) Component();
		entity.addComponent<Component>();
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

	System(std::string const& _name, Tag _signature) : name(_name), signature(_signature) {
		std::cout << "Created a" << name << "\n";
	}

	bool matchesSignature(Tag components) {
		std::cout << "components " << components << "\n";
		std::cout << "signature  " << signature << "\n";
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

struct AnouncePositionSystem : System {
	AnouncePositionSystem() : System("AnouncePositionSystem", tag<Anounce>() | tag<Position>()) {}

	void handleEntity(Entity & e, Components components) override {
		auto p = components.get<Position>(e.id);
		std::cout << "I am " << e.id << " at the position " << p->pos.x << ", " << p->pos.y;
		// TODO how do we even store string data?
		// std::cout << "ANOUNCE MESSAGE " << (const char*)(components.get<Anounce>(e.id)) << "\n";
	}
};

struct MoveSystem : System {
	MoveSystem() : System("MoveSystem", tag<Position>() | tag<Velocity>()) {}

	void handleEntity(Entity & e, Components components) override {
		// TODO update the position of the entity using position and velocity
		std::cout << "Updating position of " << e.id << "\n";
		std::cout << "Using velocity x: " << components.get<Velocity>(e.id)->vel.x << "\n";
		std::cout << "Pos x before: " << components.get<Position>(e.id)->pos.x << "\n";
		components.get<Position>(e.id)->pos.x += components.get<Velocity>(e.id)->vel.x;
		std::cout << "Pos x after:  " << components.get<Position>(e.id)->pos.x << "\n";
	}
};

struct CollisionSystem : System {
	CollisionSystem() : System("CollisionSystem", tag<Position>() | tag<Physical>() | tag<Size>()) {}

	void handleEntity(Entity & e, Components components) override {
		// TODO Update collision quadtree?
		// TODO create collision event?
		std::cout << "Checking for collisions of " << e.id << "\n";
	}
};

struct RenderSystem : System {
	RenderSystem() : System("RenderSystem", tag<Position>() | tag<Shape>()) {}

	void handleEntity(Entity & e, Components components) override {
		// TODO render to the scene with the shape and position of the entity
		std::cout << "Rendering " << e.id << "\n";
	}
};

typedef std::vector<Entity> EntityList;

struct Entities {
	EntityList entityList;

	Entities() {}

	// TODO Reuse and give the entity its ID here
	Entity& create() {
		entityList.push_back(Entity());
		std::cout << "There are now " << entityList.size() << " entities!" << "\n";
		return entityList.back();
	}
};

struct Systems {
	std::vector<System*> systemList;

	void add(System* system) {
		systemList.push_back(system);
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

	Systems systems;
	systems.add(new AnouncePositionSystem);
	systems.add(new MoveSystem);
	systems.add(new AnouncePositionSystem);
	systems.add(new RenderSystem);

	while (true) {
		std::cout << "#####################################\n\n";
		for (System * s : systems.systemList) {
			std::cout << "System : " << s->name << "\n";
			for (auto e : entities.entityList) {
				s->update(e, components);
			}
			std::cout << "\n";
		}
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}

