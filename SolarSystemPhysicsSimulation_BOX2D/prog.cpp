#include <SFML/Graphics.hpp>
#include <box2d/box2d.h>

struct Planet {
	sf::CircleShape shape;
	sf::VertexArray trail; // Trail made of lines
	b2Body* body;
};

std::vector<Planet> planets;

sf::Color getRandomColor() {
	return sf::Color(rand() % 256, rand() % 256, rand() % 256); 
}

void spawnPlanet(sf::Vector2f position, b2World& world, sf::Vector2f sunPosition) {
	Planet planet;

	sf::Color randomColor = getRandomColor();

	// SFML shape setup
	planet.shape.setRadius(10.0f);
	planet.shape.setOrigin(10.0f, 10.0f);
	planet.shape.setFillColor(randomColor);
	planet.shape.setPosition(position);

	// Box2D body setup
	b2BodyDef bodyDef;
	bodyDef.type = b2_dynamicBody;
	bodyDef.position.Set(position.x / 30.0f, position.y / 30.0f); // Convert to Box2D units
	planet.body = world.CreateBody(&bodyDef);

	b2CircleShape circle;
	circle.m_radius = 10.0f / 30.0f;

	b2FixtureDef fixtureDef;
	fixtureDef.shape = &circle;
	fixtureDef.density = 1.0f;
	fixtureDef.friction = 0.0f;
	fixtureDef.restitution = 0.0f; // No bounce
	planet.body->CreateFixture(&fixtureDef);

	// Calculate initial tangential velocity for stable orbit
	sf::Vector2f direction = position - sunPosition;
	float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

	// Normalize direction and calculate perpendicular vector
	sf::Vector2f tangent(-direction.y / distance, direction.x / distance); // Perpendicular direction

	// Use orbital velocity formula
	float gravityConstant = 100.0f; // Gravitational constant
	float sunMass = 10000.0f;       // Effective sun mass
	float orbitalSpeed = std::sqrt(gravityConstant * sunMass / distance); // Orbital velocity

	// Set initial velocity
	b2Vec2 velocity(tangent.x * orbitalSpeed / 30.0f, tangent.y * orbitalSpeed / 30.0f); // Scale to Box2D units
	planet.body->SetLinearVelocity(velocity);

	// Trail setup
	planet.trail.setPrimitiveType(sf::LineStrip);

	planets.push_back(planet);
}



void applyGravity(Planet& planet, sf::Vector2f sunPosition) {
	b2Vec2 planetPos = planet.body->GetPosition();
	sf::Vector2f planetPosition(planetPos.x * 30.0f, planetPos.y * 30.0f); // Convert to SFML units

	// Calculate direction and distance to the sun
	sf::Vector2f direction = sunPosition - planetPosition;
	float distanceSquared = direction.x * direction.x + direction.y * direction.y;

	// Avoid singularities by clamping minimum distance
	if (distanceSquared < 100.0f)
		distanceSquared = 100.0f;

	// Calculate gravitational force magnitude using inverse square law
	float gravityConstant = 5.0f; // Adjust for simulation scale
	float sunMass = 5000.0f;       // Effective sun mass
	float gravityStrength = (gravityConstant * sunMass) / distanceSquared;

	// Normalize direction and calculate force vector
	float distance = std::sqrt(distanceSquared);
	b2Vec2 force(direction.x / distance * gravityStrength, direction.y / distance * gravityStrength);

	// Apply force to the planet
	planet.body->ApplyForceToCenter(force, true);
}

void updatePlanets(b2World& world, sf::Vector2f sunPosition, float deltaTime) {
	for (size_t i = 0; i < planets.size(); ++i) {
		Planet& planet = planets[i];

		// Apply gravitational pull toward the sun
		applyGravity(planet, sunPosition);

		// Update the planet's position in SFML
		b2Vec2 planetPos = planet.body->GetPosition();
		planet.shape.setPosition(planetPos.x * 30.0f, planetPos.y * 30.0f);

		// Add trail point
		planet.trail.append(sf::Vertex(planet.shape.getPosition(), planet.shape.getFillColor()));

		// Check for destruction if the planet gets too close to the sun
		float distance = std::sqrt(std::pow(sunPosition.x - planet.shape.getPosition().x, 2) +
			std::pow(sunPosition.y - planet.shape.getPosition().y, 2));
		if (distance < 60.0f) { // Destroy if too close to the sun
			world.DestroyBody(planet.body);
			planets.erase(planets.begin() + i);
			--i;
		}
	}
}


void renderPlanets(sf::RenderWindow& window) {
	for (const auto& planet : planets) {
		window.draw(planet.trail); // Draw trail first
		window.draw(planet.shape); // Draw planet on top
	}
}


int main() {

	sf::RenderWindow window(sf::VideoMode(800, 600), "Solar System Simulation");
	sf::View view(sf::FloatRect(0, 0, 800, 600)); // Default view
	sf::Clock clock;
	b2Vec2 gravity(0, 0); // No global gravity
	b2World world(gravity);


	sf::CircleShape sun;
	sun.setRadius(50.0f);
	sun.setOrigin(50.0f, 50.0f); // Center origin
	sun.setPosition(400.0f, 300.0f); // Center of the screen
	sun.setFillColor(sf::Color::Yellow);


	while (window.isOpen()) {
		sf::Event event;
		while (window.pollEvent(event)) {

			if (event.type == sf::Event::Resized) {
				// Update the view with the new window size
				view.setSize(event.size.width, event.size.height);
				window.setView(view);
			}

			if (event.type == sf::Event::Closed)
				window.close();
			if (event.type == sf::Event::MouseButtonPressed) {
				if (event.mouseButton.button == sf::Mouse::Left) {
					// Convert mouse coordinates to world coordinates
					sf::Vector2f worldPos = window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y));
					spawnPlanet(worldPos, world, sun.getPosition());
				}
			}
		}

		float deltaTime = clock.restart().asSeconds();

		// Update
		world.Step(deltaTime, 8, 3);
		updatePlanets(world, sun.getPosition(), deltaTime);

		// Render
		window.clear(sf::Color::Black);
		window.draw(sun);
		renderPlanets(window);
		window.display();
	}

}
