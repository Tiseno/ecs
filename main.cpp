#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <string_view>

typedef unsigned int Tag;
typedef size_t EntityId;

EntityId ID = 10000;

constexpr const size_t POSITION_COMPONENT = 0;
constexpr const size_t VELOCITY_COMPONENT = 1;
constexpr const size_t SHAPE_COMPONENT    = 2;
constexpr const size_t PHYSICAL_COMPONENT = 3;
constexpr const size_t SIZE_COMPONENT     = 4;
constexpr const size_t ANOUNCE_COMPONENT  = 5;

template<size_t componentId>
inline Tag tag() {
	return 1 << componentId;
}

typedef double Z;

// NOTE ML this is a bug in cppcheck
// if declared struct, a false positive of unusedStructMember is reported
struct Vec3 { Z x, y, z; };
struct Color { Z r, g, b; };
struct Physical { Z mass; };
struct Anounce { std::string greeting; };

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

	explicit Entity() : id(++ID), components(0) {}
	explicit Entity(Tag _components) : id(++ID), components(_components) {}
};

struct Components {
	std::vector<ComponentPool*> componentPools;

	Components() {}


	void init(Entity const& e) {
		// TODO ML init all components defined by the entity components tag
	}

	template<size_t componentId>
	void assign(EntityId entityId) {
		std::cout << "Assigning component " << componentId << " to " << entityId << "\n";
		if (componentPools.size() <= componentId) {
			componentPools.resize(componentId + 1, nullptr);
		}
		if (componentPools[componentId] == nullptr) {
			componentPools[componentId] = new ComponentPool(tag<componentId>(), componentSize<componentId>());
		}
		initComponent<componentId>((*componentPools[componentId])[entityId]);
	}

	template<size_t componentId>
	constexpr size_t componentSize() {
		switch (componentId) {
			case POSITION_COMPONENT:
			case VELOCITY_COMPONENT:
			case SIZE_COMPONENT:
				return sizeof(Vec3);
			case SHAPE_COMPONENT:
				return sizeof(Color);
			case PHYSICAL_COMPONENT:
				return sizeof(Physical);
			case ANOUNCE_COMPONENT:
				return sizeof(Anounce);
		}
	}

	template<size_t componentId>
	void initComponent(void* d) {
		switch (componentId) {
			case POSITION_COMPONENT:
			case VELOCITY_COMPONENT:
			case SIZE_COMPONENT:
				new (d) Vec3();
				break;
			case SHAPE_COMPONENT:
				new (d) Color();
				break;
			case PHYSICAL_COMPONENT:
				new (d) Physical();
				break;
			case ANOUNCE_COMPONENT:
				new (d) Anounce();
				break;
		}
	}
};

struct System {
	std::string name;
	Tag signature;

	System(std::string const& _name, Tag _signature) : name(_name), signature(_signature) {
		std::cout << "Created a" << name << "\n";
	}

	bool matchesSignature(Tag components) {
		return (components & signature) == signature;
	}

	// void update(Components, Entities) {
	// TODO create iterator for all entities with this systems components
	void update(Entity & e) {
		if (!matchesSignature(e.components)) {
			return;
		}
		handleEntity(e);
	}

	virtual void handleEntity(Entity & e) = 0;
};

struct AnouncePositionSystem : System {
	AnouncePositionSystem() : System("AnouncePositionSystem", ANOUNCE_COMPONENT | POSITION_COMPONENT) {}

	void handleEntity(Entity & e) override {
		// TODO get anounce and position
		// if we manage to encode this in the template
		// we should be able to do Component<AnounceComponent>(e) and Component<PositionComponent>(e)
		// or something
		std::cout << "I am " << e.id << " at the position 0,0\n";
	}
};

struct MoveSystem : System {
	MoveSystem() : System("MoveSystem", POSITION_COMPONENT | VELOCITY_COMPONENT) {}

	void handleEntity(Entity & e) override {
		// TODO update the position of the entity using position and velocity
		std::cout << "Updating position of " << e.id << "\n";
	}
};

struct CollisionSystem : System {
	CollisionSystem() : System("CollisionSystem", POSITION_COMPONENT | PHYSICAL_COMPONENT | SIZE_COMPONENT) {}

	void handleEntity(Entity & e) override {
		// TODO Update collision quadtree?
		// TODO create collision event?
		std::cout << "Checking for collisions of " << e.id << "\n";
	}
};

struct RenderSystem : System {
	RenderSystem() : System("RenderSystem", POSITION_COMPONENT | SHAPE_COMPONENT) {}

	void handleEntity(Entity & e) override {
		// TODO render to the scene with the shape and position of the entity
		std::cout << "Rendering " << e.id << "\n";
	}
};

typedef std::vector<Entity> EntityList;

struct Entities {
	EntityList entityList;

	Entities() {}

	Entity const& create(Tag components) {
		// std::cout << "Adding entity " << e.id << "\n";
		entityList.push_back(Entity(components));
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

	auto ne = entities.create(ANOUNCE_COMPONENT);
	components.assign<ANOUNCE_COMPONENT>(ne.id);

	auto ne2 = entities.create(ANOUNCE_COMPONENT | POSITION_COMPONENT | VELOCITY_COMPONENT | SHAPE_COMPONENT);
	components.assign<ANOUNCE_COMPONENT>(ne2.id);
	components.assign<POSITION_COMPONENT>(ne2.id);
	components.assign<VELOCITY_COMPONENT>(ne2.id);
	components.assign<SHAPE_COMPONENT>(ne2.id);

	auto ne3 = entities.create(ANOUNCE_COMPONENT | POSITION_COMPONENT);
	components.assign<ANOUNCE_COMPONENT>(ne3.id);
	components.assign<POSITION_COMPONENT>(ne3.id);

	Systems systems;
	systems.add(new AnouncePositionSystem);
	systems.add(new MoveSystem);
	systems.add(new AnouncePositionSystem);
	systems.add(new RenderSystem);

	while (true) {
		std::cout << "#####################################\n\n";
		for (System * s : systems.systemList) {
			std::cout << "System : " << s->name << "\n";
			// s->update(scene)
			for (auto e : entities.entityList) {
				s->update(e);
			}
			std::cout << "\n";
		}
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}

