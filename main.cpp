// #define DEBUG_ITERATOR

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
	size_t componentSize;
	size_t totalSize;
	char * data;

	ComponentPool(std::string const& _name, ComponentId _id, size_t _componentSize) :
		name(_name),
		componentSize(_componentSize),
		totalSize(componentSize * MAX_ENTITIES)
	{
		data = new char[totalSize];
	}

	template<typename Component>
	explicit ComponentPool(Component const& dummy) : ComponentPool(Component::NAME, id<Component>(), sizeof(Component)) {}

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

std::ostream &operator<<(std::ostream &os, ComponentPool const& m) { return os << ANSI_FG_GREEN << "ComponentPool{" << m.name << " " << static_cast<float>(m.totalSize) / 1000000 << "MB " << "}" << ANSI_RESET; }

typedef size_t EntityIndex;
typedef unsigned int EntityVersion;

const EntityIndex INVALID_ENTITY_INDEX(-1);

struct EntityId {
	EntityIndex index;
	EntityVersion version;

	bool isValid() const {
		return index != INVALID_ENTITY_INDEX;
	}
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
			ComponentPool* newPool = new ComponentPool(init); // Init is only passed for the template
			componentPools[id<Component>()] = newPool;
			std::cout << newStr << *newPool << "\n";
		}
		auto cp = new ((*componentPools[id<Component>()])[entity.id.index]) Component(init);
		entity.addComponent<Component>();
		std::cout << assignStr << *cp << " to " << entity << "\n";
		return cp;
	}

	template<typename Component>
	Component* get(Entity const& entity) {
		if((entity.components & tag<Component>()) != tag<Component>()) {
			return nullptr;
		}
		return (Component*)(*componentPools[id<Component>()])[entity.id.index];
	}
};

typedef std::vector<Entity> EntityList;
typedef std::vector<EntityIndex> EntityIndexList;

struct Entities {
private:
	EntityList entityList;
	EntityIndexList freeEntityIndexes;
public:
	EntityIndexList const& freeList() const {
		return freeEntityIndexes;
	}

	EntityList const& list() const {
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
			std::cout << newStr << e << " there are now " << entityList.size() << "\n";
			return e;
		} else {
			entityList.push_back(Entity(EntityId{entityList.size(), 0}));
			Entity& e = entityList.back();
			std::cout << newStr << e << " there are now " << entityList.size() << "\n";
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
	}

	struct View {
		EntityList& entityList;
		Tag tag;
		View(EntityList& _entityList, Tag _tag) : entityList(_entityList), tag(_tag) { }

		struct Iterator {
			EntityList& entityList;
			Tag tag;
			Entity * entity;
			explicit Iterator(EntityList& _entityList, Tag _tag, Entity * _entity) : entityList(_entityList), tag(_tag), entity(_entity) {
#ifdef DEBUG_ITERATOR
				std::cout << newStr << "Iterator " << bits(tag) << " " << entity << "\n";
#endif
			}
			Entity& operator*() const {
#ifdef DEBUG_ITERATOR
				std::cout << "Dereference iterator returning " << *entity << "\n";
#endif
			return *entity;
			}
			bool operator==(Iterator const& other) const {
#ifdef DEBUG_ITERATOR
				std::cout << "Comparing iterator: " << entity->id << " == " << other.entity->id << " " << (entity->id.index == other.entity->id.index && entity->id.version == other.entity->id.version) << "\n";
#endif
				return entity->id.index == other.entity->id.index && entity->id.version == other.entity->id.version;
			}
			bool operator!=(Iterator const& other) const {
#ifdef DEBUG_ITERATOR
				std::cout << "Comparing iterator: " << entity->id << " != " << other.entity->id << " " << (entity->id.index != other.entity->id.index || entity->id.version != other.entity->id.version) << "\n";
#endif
				return entity->id.index != other.entity->id.index || entity->id.version != other.entity->id.version;
			}
			Iterator& operator++() {
				if (!entity->isValid()) {
					return *this;
				}
#ifdef DEBUG_ITERATOR
				std::cout << "Finding next iterator\n";
#endif
				for(size_t i = entity->id.index + 1; i < entityList.size(); i++) {
#ifdef DEBUG_ITERATOR
					std::cout << "\t" << i << ".\n";
#endif
					if(entityList[i].isValid() && entityList[i].matchesSignature(tag)) {
#ifdef DEBUG_ITERATOR
						std::cout << "\tentity        " << entityList[i] << "\n";
						std::cout << "\tentityList[i] " << *entity << "\n";
#endif
						entity = &entityList[i];
#ifdef DEBUG_ITERATOR
						std::cout << "\tReturning " << *entity << "\n";
#endif
						return *this;
					}
				}
				entity = &INVALID_ENTITY;
#ifdef DEBUG_ITERATOR
				std::cout << "\tReturning " << entity << "\n";
#endif
				return *this;
			}
		};
		const Iterator begin() const {
#ifdef DEBUG_ITERATOR
			std::cout << "Iterator begin: finding first matching valid entity\n";
#endif
			for(auto& e : entityList) {
#ifdef DEBUG_ITERATOR
				std::cout << "Entity " << e;
#endif
				if(e.isValid() && e.matchesSignature(tag)) {
#ifdef DEBUG_ITERATOR
					std::cout << " matched!\n";
#endif
					return Iterator(entityList, tag, &e);
				}
#ifdef DEBUG_ITERATOR
				std::cout << " did not match...\n";
#endif
			}
#ifdef DEBUG_ITERATOR
			std::cout << "\tReturning INVALID_ENTITY iterator\n";
#endif
			return Iterator(entityList, tag, &INVALID_ENTITY);
		}
		const Iterator end() const {
#ifdef DEBUG_ITERATOR
			std::cout << "Iterator end: Returning INVALID_ENTITY iterator\n";
#endif
			return Iterator(entityList, tag, &INVALID_ENTITY);
		}
	};

	View view(Tag _tag) { return View(entityList, _tag); }
};

Entity Entities::INVALID_ENTITY(INVALID_ENTITY_ID);

std::ostream &operator<<(std::ostream &os, Entities const& m) {
	os << ANSI_FG_PINK << "Entities{" << m.list().size() << " " << m.list().size() * sizeof(Entity) << "B " << " [ ";
	for(auto free : m.freeList()) {
		os << free << " ";
	}
	return os << "]}" << ANSI_RESET;
}

struct System {
	std::string name;
	Tag signature;

	System(std::string const& _name, Tag _signature) : name(_name), signature(_signature) {}

	virtual void updateAll(Entities& entities, Components& components) = 0;
};

std::ostream &operator<<(std::ostream &os, System const& m) { return os << ANSI_FG_CYAN << "System{" << m.name << " " << bits(m.signature) << "}" << ANSI_RESET; }

struct Systems {
	std::vector<System*> systemList;

	void add(System* system) {
		systemList.push_back(system);
		std::cout << newStr << *systemList.back() << "\n";
	}
};

struct TrackPositionSystem : System {
	TrackPositionSystem() : System("TrackPosition", tag<Position>()) {}

	void updateAll(Entities& entities, Components& components) override {
		for(Entity& e : entities.view(signature)) {
			components.get<Position>(e);
		}
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

	void updateAll(Entities& entities, Components& components) override {
		for(Entity& e : entities.view(signature)) {
			std::cout << e << " ";
			printComponents(e, components);
			std::cout << "\n";
		}
		std::cout << entities << "\n";
		std::cout << ANSI_FG_CYAN_DARKER << "\n#####################################\n\n" << ANSI_RESET;
	}
};

struct MoveSystem : System {
	MoveSystem() : System("Move", tag<Position>() | tag<Velocity>()) {}

	void updateAll(Entities& entities, Components& components) override {
		for(Entity& e : entities.view(signature)) {
			components.get<Position>(e)->pos.x += components.get<Velocity>(e)->vel.x;
			components.get<Position>(e)->pos.y += components.get<Velocity>(e)->vel.y;
		}
	}
};

struct CollisionSystem : System {
	CollisionSystem() : System("Collision", tag<Position>() | tag<Physical>() | tag<Size>()) {}

	void updateAll(Entities& entities, Components& components) override {
		for(Entity& e : entities.view(signature)) {
			components.get<Position>(e);
			components.get<Physical>(e);
			components.get<Size>(e);
			// TODO handle collisions
		}
	}
};

struct RenderSystem : System {
	RenderSystem() : System("Render", tag<Position>() | tag<Shape>()) {}

	void updateAll(Entities& entities, Components& components) override {
		for(Entity& e : entities.view(signature)) {
			components.get<Position>(e);
			components.get<Shape>(e);
			// TODO render to the scene with the shape and position of the entity
		}
	}
};

void createJesus(Components& components, Entities& entities) {
		static size_t jesusCounter = 0;
		Entity& ne = entities.create();
		components.assign(ne, Type("Jesus", jesusCounter++));
		components.assign(ne, Brain(100000));
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
		components.assign(ne, Inspect());

		Entity& ne2 = entities.create();
		components.assign(ne2, Type("Dumran", 1));
		components.assign(ne2, Position());
		components.assign(ne2, Velocity{0.1,0.1,0.2});
		components.assign(ne2, Shape{0,0,0});
		components.assign(ne2, Inspect());

		Entity& ne3 = entities.create();
		components.assign(ne3, Position());
		components.assign(ne3, Inspect());

		Entity& ne4 = entities.create();
		components.assign(ne4, Position());
		components.assign(ne4, Velocity());
		components.assign(ne4, Shape());
		components.assign(ne4, Physical());
		components.assign(ne4, Size());
		components.assign(ne4, Brain{50});
		components.assign(ne4, Inspect());
	}
	int count = 0;

	EntityId eid = INVALID_ENTITY_ID;
	while (true) {
		count++;

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
				std::cout << "Picked new target " << eid << " for growth\n";
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
					std::cout << ANSI_FG_ORANGE << "Size of " << ANSI_RESET << eee2 << " grew by 0.1!\n";
				} else {
					std::cout << ANSI_FG_GRAY << "Did not grow " << ANSI_RESET  << eee2 << " as it didnt have a Size!\n";
				}
			} else {
				std::cout << ANSI_FG_GRAY << "Did not grow " << ANSI_RESET  << eid << " " << eee2 << " as it is invalid!\n";
			}
		}

		{
			Entity const& eee4 = entities.getRandom();
			Brain* b = components.get<Brain>(eee4);
			if (b != nullptr) {
				b->increase(0.1);
				std::cout << ANSI_FG_ORANGE << "Increased brain power of entity " << ANSI_RESET  << eee4 << "!\n";
			} else {
				std::cout << ANSI_FG_GRAY << "The brain of " << ANSI_RESET  << eee4 << " is unallocated!\n";
			}
		}

		createLord(components, entities);
		createJesus(components, entities);
		for (System * s : systems.systemList) {
			s->updateAll(entities, components);
		}
		for(auto e : entities.list()) {
			std::cout << e;
			printComponents(e, components);
			std::cout << "\n";
		}
		std::cout << ANSI_FG_CYAN_DARKER << "\n#####################################\n\n" << ANSI_RESET;
		std::this_thread::sleep_for(std::chrono::milliseconds(400));
	}
}

