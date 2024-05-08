#include <GLFW\glfw3.h>
#include "linmath.h"
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <iostream>
#include <vector>
#include <windows.h>
#include <time.h>

using namespace std;

const float DEG2RAD = 3.14159 / 180;

void processInput(GLFWwindow* window);

enum BRICKTYPE { REFLECTIVE, DESTRUCTABLE };
enum ONOFF { ON, OFF };

class Brick
{
public:
    float red, green, blue;
    float x, y, width;
    BRICKTYPE brick_type;
    ONOFF onoff;

    Brick(BRICKTYPE bt, float xx, float yy, float ww, float rr, float gg, float bb)
    {
        brick_type = bt; x = xx; y = yy, width = ww; red = rr, green = gg, blue = bb;
        onoff = ON;
    };

    void drawBrick()
    {
        if (onoff == ON)
        {
            double halfside = width / 2;

            glColor3d(red, green, blue);
            glBegin(GL_POLYGON);

            glVertex2d(x + halfside, y + halfside);
            glVertex2d(x + halfside, y - halfside);
            glVertex2d(x - halfside, y - halfside);
            glVertex2d(x - halfside, y + halfside);

            glEnd();
        }
    }

    //added this function to change color when hit
    void ChangeColor(float r, float g, float b)
    {
        red = r;
        green = g;
        blue = b;
    }
};

class Paddle : public Brick //added this class to make the paddle at the bottom
{
public:
    Paddle(float xx, float yy, float ww, float rr, float gg, float bb)
        : Brick(REFLECTIVE, xx, yy, ww, rr, gg, bb) {} //create paddle as a type of white brick
    //movmeent
    void moveLeft()
    {
        if (x > -1 + width)
            x -= 0.05;
    }

    void moveRight()
    {
        if (x < 1 - width)
            x += 0.05;
    }
};

class Circle
{
public:
    float red, green, blue;
    float radius;
    float x;
    float y;
    float speed = 0.06;
    int direction; // 1=up 2=right 3=down 4=left 5=up right 6=up left 7=down right 8=down left

    Circle(double xx, double yy, double rr, int dir, float rad, float r, float g, float b)
    {
        x = xx;
        y = yy;
        radius = rr;
        red = r;
        green = g;
        blue = b;
        radius = rad;
        direction = dir;
    }
    //altered code to use ChangeColor to alter bick on collision

    void CheckCollision(Brick* brk)
    {
        if (brk->brick_type == REFLECTIVE)
        {
            if ((x > brk->x - brk->width && x <= brk->x + brk->width) && (y > brk->y - brk->width && y <= brk->y + brk->width))
            {
                direction = GetRandomDirection();
                x = x + 0.03;
                y = y + 0.04;

                // Change the color of the brick on collision
                float r = static_cast<float>(rand()) / RAND_MAX;//random color when hit
                float g = static_cast<float>(rand()) / RAND_MAX;
                float b = static_cast<float>(rand()) / RAND_MAX;
                brk->ChangeColor(r, g, b);
            }
        }
        else if (brk->brick_type == DESTRUCTABLE)
        {
            if ((x > brk->x - brk->width && x <= brk->x + brk->width) && (y > brk->y - brk->width && y <= brk->y + brk->width))
            {
                brk->onoff = OFF;

                // Change the color of the brick on collision
                float r = static_cast<float>(rand()) / RAND_MAX;//had to use static_case because I was getting a int error
                float g = static_cast<float>(rand()) / RAND_MAX;
                float b = static_cast<float>(rand()) / RAND_MAX;
                brk->ChangeColor(r, g, b);
            }
        }
    }

    void CheckCircleCollision(Circle* other)
    {
        float dx = x - other->x;
        float dy = y - other->y;
        float distance = sqrt(dx * dx + dy * dy);

        if (distance <= radius + other->radius)
        {
            // Change the color of the colliding circles
            red = static_cast<float>(rand()) / RAND_MAX;
            green = static_cast<float>(rand()) / RAND_MAX;
            blue = static_cast<float>(rand()) / RAND_MAX;

            other->red = static_cast<float>(rand()) / RAND_MAX;
            other->green = static_cast<float>(rand()) / RAND_MAX;
            other->blue = static_cast<float>(rand()) / RAND_MAX;
        }
    }
    int GetRandomDirection()
    {
        return (rand() % 8) + 1;
    }

    void MoveOneStep() //added to this function to make the circles speed up when they hit the walls
    {
        if (direction == 1 || direction == 5 || direction == 6)  // up
        {
            if (y > -1 + radius)
            {
                y -= speed; //decrease
            }
            else
            {
                direction = GetRandomDirection();
                speed = (rand() % 6 + 1) * 0.01;// change speed
            }
        }

        if (direction == 2 || direction == 5 || direction == 7)  // right
        {
            if (x < 1 - radius)
            {
                x += speed;
            }
            else
            {
                direction = GetRandomDirection();
                speed = (rand() % 6 + 1) * 0.01;
            }
        }

        if (direction == 3 || direction == 7 || direction == 8)  // down
        {
            if (y < 1 - radius) {
                y += speed;
            }
            else
            {
                direction = GetRandomDirection();
                speed = (rand() % 6 + 1) * 0.01;// change speed randomly
            }
        }

        if (direction == 4 || direction == 6 || direction == 8)  // left
        {
            if (x > -1 + radius) {
                x -= speed;
            }
            else
            {
                direction = GetRandomDirection();
                speed = (rand() % 6 + 1) * 0.01;
            }
        }
    }

    void DrawCircle()
    {
        glColor3f(red, green, blue);
        glBegin(GL_POLYGON);
        for (int i = 0; i < 360; i++) {
            float degInRad = i * DEG2RAD;
            glVertex2f((cos(degInRad) * radius) + x, (sin(degInRad) * radius) + y);
        }
        glEnd();
    }
};



vector<Circle> world;

void processInput(GLFWwindow* window, Paddle& paddle) //added the paddle here
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
    {
        double r, g, b;
        r = rand() / 10000.0;
        g = rand() / 10000.0;
        b = rand() / 10000.0;
        Circle B(0, 0, 02, 2, 0.05, r, g, b);
        world.push_back(B);
    }
    //add paddle movement

    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        paddle.moveLeft();

    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        paddle.moveRight();
}

int main(void) {
    srand(time(NULL));

    if (!glfwInit()) {
        exit(EXIT_FAILURE);
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    GLFWwindow* window = glfwCreateWindow(480, 480, "8-2 Assignment", NULL, NULL);
    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    Paddle paddle(0, -0.8, 0.2, 1, 1, 1);//add paddle
    Brick brick1(REFLECTIVE, 0, 0.6, 0.2, 1, 1, 0); //change colors and positons
    Brick brick2(DESTRUCTABLE, -0.3, 0.4, 0.2, 0, 1, 0);
    Brick brick3(DESTRUCTABLE, 0.3, 0.4, 0.2, 0, 1, 1);
    Brick brick4(REFLECTIVE, -0.6, 0.2, 0.2, 1, 0.5, 0.5);
    Brick brick5(REFLECTIVE, 0.6, 0.2, 0.2, 1, 0.5, 0.5);

    while (!glfwWindowShouldClose(window)) {
        //Setup View
        float ratio;
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        ratio = width / (float)height;
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);

        processInput(window, paddle);

        //Movement
        for (int i = 0; i < world.size(); i++)
        {
            world[i].CheckCollision(&paddle);
            world[i].CheckCollision(&brick1);
            world[i].CheckCollision(&brick2);
            world[i].CheckCollision(&brick3);
            world[i].CheckCollision(&brick4);
            world[i].CheckCollision(&brick5);
            world[i].MoveOneStep();
            world[i].DrawCircle();
        }
        //added this to check the circles and change their colors
        for (int i = 0; i < world.size(); i++)
        {
            for (int j = i + 1; j < world.size(); j++)
            {
                world[i].CheckCircleCollision(&world[j]);
            }
        }

        paddle.drawBrick();
        brick1.drawBrick();
        brick2.drawBrick();
        brick3.drawBrick();
        brick4.drawBrick();
        brick5.drawBrick();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}