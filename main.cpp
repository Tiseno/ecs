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

typedef size_t EntityIndex;
typedef unsigned int EntityVersion;

const EntityIndex INVALID_ENTITY_INDEX(-1);

struct EntityId {
	EntityIndex index;
	EntityVersion version;

	bool isValid() const {
		return index != INVALID_ENTITY_INDEX;
	}

	// bool operator!=(EntityId& other) {
	// 	return !(other.index == index && other.version == version);
	// }
};

std::ostream &operator<<(std::ostream &os, EntityId const& m) {
	os << ANSI_FG_GRAY << "EntityId{";
	if (m.index == INVALID_ENTITY_INDEX) {
		os << "INVALID";
	} else {
		os << m.index;
	}
	return os << " " << m.version << "}" << ANSI_RESET;
}

const EntityId INVALID_ENTITY_ID{INVALID_ENTITY_INDEX, 0};

struct Entity {
	EntityId id;
	Tag components;

	static EntityIndex ENTITY_INDEX;

	explicit Entity(EntityId _id) : id(_id), components(0) {}

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

	bool isValid() const {
		return id.isValid();
	}

	bool matchesSignature(Tag signature) {
		return (components & signature) == signature;
	}
};

EntityIndex Entity::ENTITY_INDEX = 0;

std::ostream &operator<<(std::ostream &os, Entity const& m) {
	os << ANSI_FG_MAGENTA << "Entity{";
	if (m.id.index == INVALID_ENTITY_INDEX) {
		os << "INVALID";
	} else {
		os << m.id.index;
	}
	return os<< " " << m.id.version << " " << bits(m.components) << "}" << ANSI_RESET;
}

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
		auto cp = new ((*componentPools[id<Component>()])[entity.id.index]) Component(init);
		entity.addComponent<Component>();
#ifdef DEBUG_2
		std::cout << assignStr << *cp << " to " << entity << "\n";
#endif
		return cp;
	}

	// TODO make this a reference and return a null component instance instead?
	template<typename Component>
	Component* get(Entity const& entity) {
		if((entity.components & tag<Component>()) != tag<Component>()) {
			return nullptr;
		}
		return (Component*)(*componentPools[id<Component>()])[entity.id.index];
	}
};

struct System {
	std::string name;
	Tag signature;

	System(std::string const& _name, Tag _signature) : name(_name), signature(_signature) {}

	// TODO create iterator for all entities with this systems components
	void update(Entity & e, Components& components) {
		if (!e.isValid() || !e.matchesSignature(signature)) {
			return;
		}
		handleEntity(e, components);
	}

	// TODO this should handle its own for loop
	virtual void handleEntity(Entity & e, Components& components) = 0;
};

std::ostream &operator<<(std::ostream &os, System const& m) { return os << ANSI_FG_CYAN << "System{" << m.name << " " << bits(m.signature) << "}" << ANSI_RESET; }

struct TrackPositionSystem : System {
	TrackPositionSystem() : System("TrackPosition", tag<Position>()) {}

	void handleEntity(Entity & e, Components& components) override {
		components.get<Position>(e);
	}
};

void printComponents(Entity& e, Components components) {
		if(components.get<Type>(e) != nullptr)         { std::cout << "\t" << ANSI_FG_MAGENTA << "|" << ANSI_RESET << (*components.get<Type>(e))         ;}
		if(components.get<Position>(e) != nullptr)     { std::cout << "\t" << ANSI_FG_MAGENTA << "|" << ANSI_RESET << (*components.get<Position>(e))     ;}
		if(components.get<Velocity>(e) != nullptr)     { std::cout << "\t" << ANSI_FG_MAGENTA << "|" << ANSI_RESET << (*components.get<Velocity>(e))     ;}
		if(components.get<Acceleration>(e) != nullptr) { std::cout << "\t" << ANSI_FG_MAGENTA << "|" << ANSI_RESET << (*components.get<Acceleration>(e)) ;}
		if(components.get<Shape>(e) != nullptr)        { std::cout << "\t" << ANSI_FG_MAGENTA << "|" << ANSI_RESET << (*components.get<Shape>(e))        ;}
		if(components.get<Physical>(e) != nullptr)     { std::cout << "\t" << ANSI_FG_MAGENTA << "|" << ANSI_RESET << (*components.get<Physical>(e))     ;}
		if(components.get<Size>(e) != nullptr)         { std::cout << "\t" << ANSI_FG_MAGENTA << "|" << ANSI_RESET << (*components.get<Size>(e))         ;}
		if(components.get<Brain>(e) != nullptr)        { std::cout << "\t" << ANSI_FG_MAGENTA << "|" << ANSI_RESET << (*components.get<Brain>(e))        ;}
		if(components.get<Inspect>(e) != nullptr)      { std::cout << "\t" << ANSI_FG_MAGENTA << "|" << ANSI_RESET << (*components.get<Inspect>(e))      ;}
}

struct InspectSystem : System {
	InspectSystem() : System("Inspect", tag<Inspect>()) {}

	void handleEntity(Entity & e, Components& components) override {
			std::cout << e << " ";
			printComponents(e, components);
			std::cout << "\n";
	}
};

struct MoveSystem : System {
	MoveSystem() : System("Move", tag<Position>() | tag<Velocity>()) {}

	void handleEntity(Entity & e, Components& components/*, double deltaTime */) override {
		components.get<Position>(e)->pos.x += components.get<Velocity>(e)->vel.x;
		components.get<Position>(e)->pos.y += components.get<Velocity>(e)->vel.y;
	}
};

struct CollisionSystem : System {
	CollisionSystem() : System("Collision", tag<Position>() | tag<Physical>() | tag<Size>()) {}

	void handleEntity(Entity & e, Components& components) override {
		components.get<Position>(e);
		components.get<Physical>(e);
		components.get<Size>(e);
		// TODO Update collision quadtree?
		// TODO create collision event?
	}
};

struct RenderSystem : System {
	RenderSystem() : System("Render", tag<Position>() | tag<Shape>()) {}

	void handleEntity(Entity & e, Components& components) override {
		components.get<Position>(e);
		components.get<Shape>(e);
		// TODO render to the scene with the shape and position of the entity
	}
};

struct Entities {
private:
	std::vector<Entity> entityList;
	std::vector<EntityIndex> freeEntityIndexes;
public:
	std::vector<EntityIndex> const& freeList() const {
		return freeEntityIndexes;
	}

	std::vector<Entity> const& list() const {
		return entityList;
	}

	static Entity INVALID_ENTITY;

	size_t size() {
		return entityList.size();
	}

	Entity const& getRandom() {
		return entityList[rand() % entityList.size()];
	}

	Entity const& operator[](EntityId id) {
		if (!id.isValid() || entityList[id.index].id.version != id.version) {
			return INVALID_ENTITY;
		}
		return entityList[id.index];
	}

	Entities() {}

	Entity& create() {
		if (freeEntityIndexes.size() > 0) {
			EntityIndex index = freeEntityIndexes.back();
			freeEntityIndexes.pop_back();
			Entity& e = entityList[index];
			e.id = EntityId{index, entityList[index].id.version + 1};
#ifdef DEBUG_2
			std::cout << newStr << e << " there are now " << entityList.size() << "\n";
#endif
			return e;
		} else {
			entityList.push_back(Entity(EntityId{entityList.size(), 0}));
			Entity& e = entityList.back();
#ifdef DEBUG_2
			std::cout << newStr << e << " there are now " << entityList.size() << "\n";
#endif
			return e;
		}
	}

	void remove(EntityId id) {
		if (!id.isValid() || entityList[id.index].id.version != id.version) {
			return;
		}
		Entity& e = entityList[id.index];
		e.id = EntityId{INVALID_ENTITY_INDEX, e.id.version};
		e.components = 0;
		freeEntityIndexes.push_back(id.index);
		// std::cout << "Removed " << id << " and pushed " << id.index << " as a free index!\n";
	}
};

Entity Entities::INVALID_ENTITY(INVALID_ENTITY_ID);

std::ostream &operator<<(std::ostream &os, Entities const& m) {
	os << ANSI_FG_PINK << "Entities{" << m.list().size() << " " << m.list().size() * sizeof(Entity) << "B " << " [ ";
	for(auto free : m.freeList()) {
		os << free << " ";
	}
	return os << "]}" << ANSI_RESET;
}

struct Systems {
	std::vector<System*> systemList;

	void add(System* system) {
		systemList.push_back(system);
#ifdef DEBUG_1
		std::cout << newStr << *systemList.back() << "\n";
#endif
	}
};

void createJesus(Components& components, Entities& entities) {
		static size_t jesusCounter = 0;
		Entity& ne = entities.create();
		components.assign(ne, Type("Jesus", jesusCounter++));
		components.assign(ne, Brain(100000));
		components.assign(ne, Inspect());
}

void createLord(Components& components, Entities& entities) {
		static size_t lordCounter = 0;
		Entity& ne = entities.create();
		components.assign(ne, Type("Lord", lordCounter++));
		components.assign(ne, Position{0,0,0});
		components.assign(ne, Size{10,10,10});
		components.assign(ne, Velocity{
				-50 + 100 * (100 / (1+(float)(rand() % 1000))),
				-50 + 100 * (100 / (1+(float)(rand() % 1000))),
				0
		});
		components.assign(ne, Acceleration());
		components.assign(ne, Brain(0));
		components.assign(ne, Inspect());
}

void removeRandomEntity(Entities& entities) {
	entities.remove(entities.getRandom().id);
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
	int count = 0;

	EntityId eid = INVALID_ENTITY_ID;
	while (true) {
		count++;

#ifdef DEBUG_2
		// TODO put this in the inspect system, when the system can handle all entities
		std::cout << ANSI_FG_CYAN_DARKER << "\n#####################################\n\n" << ANSI_RESET;
#endif

		{
			if (count % 5 == 0) {
				for(float i = 1.0; i < ((float)(entities.size()) * 0.80); i += 1) {
					removeRandomEntity(entities);
				}
			}
		}

		{
			if (count % 6 == 0) {
				int bcount = 0;
				do {
					Entity const& eee = entities.getRandom();
					eid = eee.id;
				} while (!eid.isValid() && ++bcount < 10);
#ifdef DEBUG_2
				std::cout << "Picked new target " << eid << " for growth\n";
#endif
			}
		}

		{
			Entity const& eee2 = entities[eid];
			if (eee2.isValid()) {
				Size* eees = components.get<Size>(eee2);
				if (eees != nullptr) {
					eees->size.x = eees->size.x + 0.1;
					eees->size.y = eees->size.y + 0.1;
					eees->size.z = eees->size.z + 0.1;
#ifdef DEBUG_2
					std::cout << ANSI_FG_ORANGE << "Size of " << ANSI_RESET << eee2 << " grew by 0.1!\n";
#endif
				} else {
#ifdef DEBUG_2
					std::cout << ANSI_FG_GRAY << "Did not grow " << ANSI_RESET  << eee2 << " as it didnt have a Size!\n";
#endif
				}
			} else {
#ifdef DEBUG_2
				std::cout << ANSI_FG_GRAY << "Did not grow " << ANSI_RESET  << eid << " " << eee2 << " as it is invalid!\n";
#endif
			}
		}

		{
			Entity const& eee4 = entities.getRandom();
			Brain* b = components.get<Brain>(eee4);
			if (b != nullptr) {
				b->increase(0.1);
#ifdef DEBUG_2
				std::cout << ANSI_FG_ORANGE << "Increased brain power of entity " << ANSI_RESET  << eee4 << "!\n";
#endif
			} else {
#ifdef DEBUG_2
				std::cout << ANSI_FG_GRAY << "The brain of " << ANSI_RESET  << eee4 << " is unallocated!\n";
#endif
			}
		}

		createLord(components, entities);
		createJesus(components, entities);
		for (System * s : systems.systemList) {
			for (auto e : entities.list()) {
				s->update(e, components);
			}
		}
		// TODO put this in the inspect system, when the system can handle all entities
#ifdef DEBUG_2
		std::cout << entities << "\n";
#endif
		std::this_thread::sleep_for(std::chrono::milliseconds(400));
	}
}

