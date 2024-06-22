#include <GLFW/glfw3.h>
#include "linmath.h"
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <memory>
#include <ctime>
#include <sqlite3.h>

using namespace std;

const float DEG2RAD = 3.14159 / 180;

// SQLite database
sqlite3* db;

// Function to initialize the SQLite database
void initDatabase() {
    if (sqlite3_open("game.db", &db)) {
        cerr << "Can't open database: " << sqlite3_errmsg(db) << endl;
        exit(1);
    }
    const char* createTableSQL = "CREATE TABLE IF NOT EXISTS userScores (userName TEXT PRIMARY KEY, score INTEGER);";
    char* errMsg = nullptr;
    if (sqlite3_exec(db, createTableSQL, 0, 0, &errMsg) != SQLITE_OK) {
        cerr << "SQL error: " << errMsg << endl;
        sqlite3_free(errMsg);
        exit(1);
    }
}

// Function to add or update a user score
void upsertUserScore(const string& userName, int score) {
    const char* upsertSQL = "INSERT INTO userScores (userName, score) VALUES (?1, ?2) "
                            "ON CONFLICT(userName) DO UPDATE SET score = excluded.score;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, upsertSQL, -1, &stmt, 0) != SQLITE_OK) {
        cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << endl;
        exit(1);
    }
    sqlite3_bind_text(stmt, 1, userName.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, score);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        cerr << "Execution failed: " << sqlite3_errmsg(db) << endl;
    }
    sqlite3_finalize(stmt);
}

// Function to get a user score
int getUserScore(const string& userName) {
    const char* selectSQL = "SELECT score FROM userScores WHERE userName = ?1;";
    sqlite3_stmt* stmt;
    int score = 0;
    if (sqlite3_prepare_v2(db, selectSQL, -1, &stmt, 0) != SQLITE_OK) {
        cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << endl;
        exit(1);
    }
    sqlite3_bind_text(stmt, 1, userName.c_str(), -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        score = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return score;
}

// Class representing a Brick in the game
class Brick {
public:
    float red, green, blue;
    float x, y, width, height;
    enum BRICKTYPE { REFLECTIVE, DESTRUCTABLE } brick_type;
    enum ONOFF { ON, OFF } onoff;

    Brick(BRICKTYPE bt, float xx, float yy, float ww, float hh, float rr, float gg, float bb)
        : brick_type(bt), x(xx), y(yy), width(ww), height(hh), red(rr), green(gg), blue(bb), onoff(ON) {}

    void drawBrick() {
        if (onoff == ON) {
            glColor3d(red, green, blue);
            glBegin(GL_POLYGON);
            glVertex2d(x + width / 2, y + height / 2);
            glVertex2d(x + width / 2, y - height / 2);
            glVertex2d(x - width / 2, y - height / 2);
            glVertex2d(x - width / 2, y + height / 2);
            glEnd();
        }
    }

    bool checkCollision(float ballX, float ballY, float ballRadius) {
        if (onoff == OFF) return false;
        return ballX + ballRadius > x - width / 2 &&
               ballX - ballRadius < x + width / 2 &&
               ballY + ballRadius > y - height / 2 &&
               ballY - ballRadius < y + height / 2;
    }
};

// Class representing a Circle (Ball)
class Circle {
public:
    float red, green, blue;
    float x, y, radius;
    float speedX, speedY;

    Circle(float xx, float yy, float rr, float rr1, float gg, float bb)
        : x(xx), y(yy), radius(rr), red(rr1), green(gg), blue(bb), speedX(0.01f), speedY(0.01f) {}

    void drawCircle() {
        glColor3d(red, green, blue);
        glBegin(GL_TRIANGLE_FAN);
        glVertex2d(x, y);
        for (int i = 0; i <= 360; i++) {
            float degInRad = i * DEG2RAD;
            glVertex2d(x + cos(degInRad) * radius, y + sin(degInRad) * radius);
        }
        glEnd();
    }

    void move() {
        x += speedX;
        y += speedY;

        if (x + radius > 1.0f || x - radius < -1.0f) {
            speedX = -speedX;
        }
        if (y + radius > 1.0f || y - radius < -1.0f) {
            speedY = -speedY;
        }
    }
};

// Class representing a Paddle
class Paddle {
public:
    float red, green, blue;
    float x, y, width, height;

    Paddle(float xx, float yy, float ww, float hh, float rr, float gg, float bb)
        : x(xx), y(yy), width(ww), height(hh), red(rr), green(gg), blue(bb) {}

    void drawBrick() {
        glColor3d(red, green, blue);
        glBegin(GL_POLYGON);
        glVertex2d(x + width / 2, y + height / 2);
        glVertex2d(x + width / 2, y - height / 2);
        glVertex2d(x - width / 2, y - height / 2);
        glVertex2d(x - width / 2, y + height / 2);
        glEnd();
    }

    bool checkCollision(float ballX, float ballY, float ballRadius) {
        return ballX + ballRadius > x - width / 2 &&
               ballX - ballRadius < x + width / 2 &&
               ballY + ballRadius > y - height / 2 &&
               ballY - ballRadius < y + height / 2;
    }
};

// Function to initialize the game
GLFWwindow* initGame() {
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    GLFWwindow* window = glfwCreateWindow(800, 600, "Brick Breaker", NULL, NULL);
    if (!window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(window);
    return window;
}

// Function to process input
void processInput(GLFWwindow* window, unique_ptr<Paddle>& paddle) {
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        paddle->x -= 0.05f;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        paddle->x += 0.05f;
    }
}

// Function to create bricks
vector<unique_ptr<Brick>> createBricks() {
    vector<unique_ptr<Brick>> bricks;
    float startX = -0.8f;
    float startY = 0.8f;
    float brickWidth = 0.2f;
    float brickHeight = 0.1f;
    int rows = 5;
    int cols = 8;

    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            float x = startX + j * brickWidth;
            float y = startY - i * brickHeight;
            bricks.push_back(make_unique<Brick>(Brick::DESTRUCTABLE, x, y, brickWidth, brickHeight, 0.0f, 1.0f, 0.0f));
        }
    }

    return bricks;
}

// Main function
int main(void) {
    srand(time(0));
    GLFWwindow* window = nullptr;

    initDatabase();

    try {
        window = initGame();
    }
    catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }

    auto paddle = make_unique<Paddle>(0.0f, -0.9f, 0.2f, 0.05f, 1.0f, 1.0f, 1.0f);
    auto ball = make_unique<Circle>(0.0f, -0.8f, 0.05f, 1.0f, 0.0f, 0.0f);

    vector<unique_ptr<Brick>> bricks = createBricks();

    std::string userName = "Player1";
    upsertUserScore(userName, 0);

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        processInput(window, paddle);

        ball->move();

        if (paddle->checkCollision(ball->x, ball->y, ball->radius)) {
            ball->speedY = -ball->speedY;
            ball->y = paddle->y + paddle->height / 2 + ball->radius;
        }

        for (auto& brick : bricks) {
            if (brick->checkCollision(ball->x, ball->y, ball->radius)) {
                brick->onoff = Brick::OFF;
                ball->speedY = -ball->speedY;
                ball->y += ball->speedY;

                int currentScore = getUserScore(userName);
                upsertUserScore(userName, currentScore + 10);
                break;
            }
        }

        paddle->drawBrick();
        ball->drawCircle();
        for (auto& brick : bricks) {
            brick->drawBrick();
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    std::cout << "Final Score for " << userName << ": " << getUserScore(userName) << std::endl;

    glfwTerminate();
    sqlite3_close(db);
    return 0;
}
