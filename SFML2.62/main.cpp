#include <SFML/Graphics.hpp>
#include <ctime>
#include <cstdlib>
#include <sstream>
#include <iostream>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 800;

const float CAR_SPEED = 600.f;
const float OBSTACLE_SPEED = 350.f;

const float CAR_SCALE = 0.15f;
const float OB_SCALE_MIN = 0.12f;
const float OB_SCALE_MAX = 0.14f;

const int NUM_LANES = 3;

class obstacle {
public:
    sf::Sprite sprite;
    bool passed = false;

    obstacle(sf::Texture& tex, float x, float y, float scale) {
        sprite.setTexture(tex);
        sprite.setScale(scale, scale);
        sprite.setPosition(x, y);
    }

    void update(float dt) {
        sprite.move(0.f, OBSTACLE_SPEED * dt);
    }

    sf::FloatRect gethitbox() const {
        sf::FloatRect hb = sprite.getGlobalBounds();
        hb.left += 8;
        hb.width -= 16;
        return hb;
    }
};

struct node {
    obstacle data;
    node* next;
    node(obstacle o) : data(o), next(nullptr) {}
};

class obstaclelist {
private:
    node* head;

public:
    obstaclelist() : head(nullptr) {}

    ~obstaclelist() {
        while (head) {
            node* temp = head;
            head = head->next;
            delete temp;
        }
    }

    void addatfront(obstacle o) {  
        node* n = new node(o);
        n->next = head;
        head = n;
    }

    void updateall(float dt) {
        node* cur = head;
        while (cur) {
            cur->data.update(dt);
            cur = cur->next;
        }
    }

    int updatescoreifpassed(float playerY) {
        int count = 0;
        node* cur = head;
        while (cur) {
            if (!cur->data.passed && cur->data.sprite.getPosition().y > playerY) {
                cur->data.passed = true;
                count++;
            }
            cur = cur->next;
        }
        return count;
    }

    void removeoffscreen() {
        node* cur = head;
        node* prev = nullptr;

        while (cur) {
            if (cur->data.sprite.getPosition().y > WINDOW_HEIGHT) {
                node* del = cur;
                if (!prev) {
                    head = cur->next;
                    cur = head;
                }
                else {
                    prev->next = cur->next;
                    cur = cur->next;
                }
                delete del;
            }
            else {
                prev = cur;
                cur = cur->next;
            }
        }
    }

    bool checkcollision(const sf::FloatRect& carBox) {
        node* cur = head;
        while (cur) {
            if (carBox.intersects(cur->data.gethitbox()))
                return true;
            cur = cur->next;
        }
        return false;
    }

    void draw(sf::RenderWindow& win) {
        node* cur = head;
        while (cur) {
            win.draw(cur->data.sprite);
            cur = cur->next;
        }
    }
};

int main() {
    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Highway Survival", sf::Style::Close);
    window.setFramerateLimit(60);

    sf::Texture cartex, obstex, bgtex, gameovertex;
    if (!cartex.loadFromFile("car.png") || !obstex.loadFromFile("o.png") ||
        !bgtex.loadFromFile("main.png") || !gameovertex.loadFromFile("gameover.png")) {
        std::cerr << "Failed to load graphic" << std::endl;
        return -1;
    }

    sf::Sprite bg(bgtex);
    bg.setScale((float)WINDOW_WIDTH / bgtex.getSize().x, (float)WINDOW_HEIGHT / bgtex.getSize().y);

    sf::Sprite gameoverbg(gameovertex);
    gameoverbg.setScale((float)WINDOW_WIDTH / gameovertex.getSize().x, (float)WINDOW_HEIGHT / gameovertex.getSize().y);

    sf::Sprite car(cartex);
    car.setScale(CAR_SCALE, CAR_SCALE);

    float lanewidth = car.getGlobalBounds().width * 2.f;
    float roadwidth = lanewidth * NUM_LANES;
    float roadx = (WINDOW_WIDTH - roadwidth) / 2.f;

    car.setPosition(roadx + roadwidth / 2.f - car.getGlobalBounds().width / 2.f,
        WINDOW_HEIGHT - car.getGlobalBounds().height - 30.f);

    obstaclelist obstacles;
    float spawntimer = 0.f;
    float spawndelay = 1.1f;

    int score = 0;
    sf::Font font;
    font.loadFromFile("arial.ttf");
    sf::Text scoretext;
    scoretext.setFont(font);
    scoretext.setCharacterSize(28);
    scoretext.setFillColor(sf::Color::White);
    scoretext.setPosition(20, 20);

    bool inmenu = true;
    bool isgameover = false;

    sf::RectangleShape playbtn({ 300, 80 });
    playbtn.setFillColor(sf::Color(40, 180, 40));
    playbtn.setPosition(WINDOW_WIDTH / 2 - 150, 450);

    sf::RectangleShape exitbtn({ 300, 80 });
    exitbtn.setFillColor(sf::Color(180, 40, 40));
    exitbtn.setPosition(WINDOW_WIDTH / 2 - 150, 550);

    sf::Text playtext("PLAY", font, 36);
    playtext.setPosition(playbtn.getPosition().x + 110, playbtn.getPosition().y + 18);

    sf::Text exittext("EXIT", font, 36);
    exittext.setPosition(exitbtn.getPosition().x + 115, exitbtn.getPosition().y + 18);

    float laneoffset = 0.f;
    const float LINE_H = 60.f;
    const float LINE_GAP = 40.f;

    sf::Clock clock;
    srand((unsigned)time(nullptr));

    while (window.isOpen()) {
        sf::Event e;
        while (window.pollEvent(e)) {
            if (e.type == sf::Event::Closed)
                window.close();

            if (inmenu && e.type == sf::Event::MouseButtonPressed) {
                auto mouse = sf::Mouse::getPosition(window);
                if (playbtn.getGlobalBounds().contains(mouse.x, mouse.y))
                    inmenu = false;
                if (exitbtn.getGlobalBounds().contains(mouse.x, mouse.y))
                    window.close();
            }
        }

        float dt = clock.restart().asSeconds();

        if (inmenu) {
            window.clear();
            window.draw(bg);
            window.draw(playbtn);
            window.draw(exitbtn);
            window.draw(playtext);
            window.draw(exittext);
            window.display();
            continue;
        }

        if (isgameover) {
            window.clear();
            window.draw(gameoverbg);

            sf::Text finalscore("Final Score: " + std::to_string(score), font, 42);
            finalscore.setFillColor(sf::Color::White);
            finalscore.setPosition(WINDOW_WIDTH / 2 - finalscore.getGlobalBounds().width / 2, 30);
            window.draw(finalscore);

            sf::RectangleShape menubtn({ 300, 80 });
            menubtn.setFillColor(sf::Color(40, 180, 40));
            menubtn.setPosition(WINDOW_WIDTH / 2 - 150, WINDOW_HEIGHT - 120);
            window.draw(menubtn);

            sf::Text menutext("Main Menu", font, 30);
            menutext.setPosition(menubtn.getPosition().x + (menubtn.getSize().x - menutext.getGlobalBounds().width) / 2, menubtn.getPosition().y + 20);
            window.draw(menutext);

            window.display();

            if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
                auto mouse = sf::Mouse::getPosition(window);
                if (menubtn.getGlobalBounds().contains(mouse.x, mouse.y)) {
                    isgameover = false;
                    inmenu = true;
                    score = 0;
                    obstacles = obstaclelist();
                    car.setPosition(roadx + roadwidth / 2.f - car.getGlobalBounds().width / 2.f,
                        WINDOW_HEIGHT - car.getGlobalBounds().height - 30.f);
                }
            }
            continue;
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
            car.move(-CAR_SPEED * dt, 0);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
            car.move(CAR_SPEED * dt, 0);

        if (car.getPosition().x < roadx)
            car.setPosition(roadx, car.getPosition().y);
        if (car.getPosition().x + car.getGlobalBounds().width > roadx + roadwidth)
            car.setPosition(roadx + roadwidth - car.getGlobalBounds().width, car.getPosition().y);

        spawntimer += dt;
        if (spawntimer >= spawndelay) {
            int lane = rand() % NUM_LANES;
            float scale = OB_SCALE_MIN + (rand() / (float)RAND_MAX) * (OB_SCALE_MAX - OB_SCALE_MIN);
            float obswidth = obstex.getSize().x * scale;
            float x = roadx + lane * lanewidth + (lanewidth - obswidth) / 2.f;

            float y = -static_cast<float>(obstex.getSize().y) * scale;

            obstacles.addatfront(obstacle(obstex, x, y, scale));  
            spawntimer = 0.f;
        }

        obstacles.updateall(dt);
        score += obstacles.updatescoreifpassed(car.getPosition().y) * 10;
        obstacles.removeoffscreen();

        if (obstacles.checkcollision(car.getGlobalBounds()))
            isgameover = true;

        window.clear();
        window.draw(bg);

        sf::RectangleShape road({ roadwidth, (float)WINDOW_HEIGHT });
        road.setPosition(roadx, 0);
        road.setFillColor(sf::Color(40, 40, 40));
        window.draw(road);

        laneoffset += OBSTACLE_SPEED * dt;
        if (laneoffset > LINE_H + LINE_GAP) laneoffset = 0;
        for (int i = 1; i < NUM_LANES; i++) {
            float lx = roadx + i * lanewidth;
            for (float y = -LINE_H - laneoffset; y < WINDOW_HEIGHT; y += LINE_H + LINE_GAP) {
                sf::RectangleShape line({ 6, LINE_H });
                line.setPosition(lx - 3, y);
                line.setFillColor(sf::Color::White);
                window.draw(line);
            }
        }

        window.draw(car);
        obstacles.draw(window);

        scoretext.setString("Score: " + std::to_string(score));
        window.draw(scoretext);

        window.display();
    }

    return 0;
}
