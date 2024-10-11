#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define GL_GLEXT_PROTOTYPES 1
#define LOG(argument) std::cout << argument << '\n'

#ifdef _WINDOWS
    #include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include <vector>
#include <ctime>
#include "cmath"

enum AppStatus { RUNNING, TERMINATED };

constexpr int WINDOW_WIDTH  = 640 * 2,
              WINDOW_HEIGHT = 800;

constexpr float BG_RED     = 0.9765625f,
                BG_GREEN   = 0.97265625f,
                BG_BLUE    = 0.9609375f,
                BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
              VIEWPORT_Y = 0,
              VIEWPORT_WIDTH  = WINDOW_WIDTH,
              VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
               F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;

constexpr char KEYBOARD_SPRITESHEET[] = "Keyboard Letters and Symbols.png",
               KEYBOARD_EXTRA_SPRITESHEET[]   = "Keyboard Extras.png",
               FONTSHEET_FILEPATH[]   = "font1.png";

constexpr GLint NUMBER_OF_TEXTURES = 1,
                LEVEL_OF_DETAIL    = 0,
                TEXTURE_BORDER     = 0;

constexpr int KEYBOARD_DIMENSIONS_COLS = 8;
constexpr int KEYBOARD_DIMENSIONS_ROWS = 14;
constexpr int KEYBOARD_EXTRA_DIMENSIONS_ROWS = 8;
constexpr int KEYBOARD_EXTRA_DIMENSIONS_COLS = 4;

constexpr int FONTBANK_SIZE = 16;

GLuint g_keyboard_texture_id;
GLuint g_keyboard_extra_texture_id;
GLuint g_font_texture_id;

float g_paddle_speed = 2.0f,
      g_ball_speed = 4.0f;

bool p1Upper = false,
     p2Upper = false,
     p1Lower = false,
     p2Lower = false,
     ballLower1 = false,
     ballUpper1 = false,
     ballLower2 = false,
     ballUpper2 = false,
     ballLower3 = false,
     ballUpper3 = false,
     gameStart = false,
     gameProgress = false,
     intialDirection = true,
     twoPlayerMode = true,
     player1Score = false,
     player2Score = false;

int numBalls = 1;

void draw_sprite_from_texture_atlas(ShaderProgram *program, GLuint texture_id, int index,
                                    int rows, int cols);

SDL_Window* g_display_window = nullptr;
AppStatus g_app_status = RUNNING;

ShaderProgram g_shader_program = ShaderProgram();

glm::mat4 g_view_matrix,
          g_ball_matrix1,
          g_ball_matrix2,
          g_ball_matrix3,
          letter_matrix,
          startGame_matrix,
          g_playerOne_matrix,
          g_playerTwo_matrix,
          g_projection_matrix;

float previous_ticks = 0.0f;

glm::vec3 INIT_PADDLE_SCALE = glm::vec3(1.0f, 0.25f, 0.0f);
glm::vec3 INIT_BALL_SCALE = glm::vec3(0.3f, 0.3f, 0.0f);
glm::vec3 INIT_PLAYERONE_POS = glm::vec3(-4.5f, 0.0f, 0.0f);
glm::vec3 INIT_PLAYERTWO_POS = glm::vec3(4.5f, 0.0f, 0.0f);
glm::vec3 INIT_BALL_POS1 = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 INIT_BALL_POS2 = glm::vec3(0.0f, 0.25f, 0.0f);
glm::vec3 INIT_BALL_POS3 = glm::vec3(0.0f, -0.25f, 0.0f);


glm::vec3 g_ball_position1 = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_ball_position2 = glm::vec3(0.0f, 0.25, 0.0f);
glm::vec3 g_ball_position3 = glm::vec3(0.0f, -0.25f, 0.0f);
glm::vec3 g_playerOne_position = glm::vec3(-4.5f, 0.0f, 0.0f);
glm::vec3 g_playerTwo_position = glm::vec3(4.5f, 0.0f, 0.0f);

glm::vec3 g_ball_movement1 = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_ball_movement2 = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_ball_movement3 = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_playerOne_movement = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_playerTwo_movement = glm::vec3(0.0f, 0.0f, 0.0f);

void initialise();
void process_input();
void update();
void render();
void shutdown();

GLuint load_texture(const char* filepath);
void draw_object(glm::mat4 &object_model_matrix, GLuint &object_texture_id);

/* Self-made funcions */
void checkYBounds(glm::vec3 object, bool &upper, bool &lower);
bool checkCollision(glm::vec3 object1, glm::vec3 object2);
int checkScore(glm::vec3 object);
void startGame();
void drawGameOver();
void drawSpaceToStart();
void drawPlayerWins();

void draw_sprite_from_texture_atlas(ShaderProgram *shaderProgram, GLuint texture_id, int index, int rows, int cols)
{
    // Step 1: Calculate the UV location of the indexed frame
    float u_coord = (float) (index % cols) / (float) cols;
    float v_coord = (float) (index / cols) / (float) rows;

    // Step 2: Calculate its UV size
    float width = 1.0f / (float) cols;
    float height = 1.0f / (float) rows;

    // Step 3: Just as we have done before, match the texture coordinates to the vertices
    float tex_coords[] =
    {
        u_coord, v_coord + height, u_coord + width, v_coord + height, u_coord + width,
        v_coord, u_coord, v_coord + height, u_coord + width, v_coord, u_coord, v_coord
    };

    float vertices[] =
    {
        -0.5, -0.5, 0.5, -0.5,  0.5, 0.5,
        -0.5, -0.5, 0.5,  0.5, -0.5, 0.5
    };

    // Step 4: And render
    glBindTexture(GL_TEXTURE_2D, texture_id);

    glVertexAttribPointer(shaderProgram->get_position_attribute(), 2, GL_FLOAT, false, 0,
                          vertices);
    glEnableVertexAttribArray(shaderProgram->get_position_attribute());

    glVertexAttribPointer(shaderProgram->get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0,
                          tex_coords);
    glEnableVertexAttribArray(shaderProgram->get_tex_coordinate_attribute());

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(shaderProgram->get_position_attribute());
    glDisableVertexAttribArray(shaderProgram->get_tex_coordinate_attribute());
}


GLuint load_texture(const char* filepath)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components,
                                     STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);

    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER,
                 GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    stbi_image_free(image);

    return textureID;
}

/* Self-made */
void checkYBounds(glm::vec3 object, bool &upper, bool &lower) {
    if (object.y > 3.2f) {
        upper = true;
    } else if (object.y < -3.2f) {
        lower = true;
    } else {
        upper = false;
        lower = false;
    }
}

bool checkCollision(glm::vec3 object1, glm::vec3 object2) {
    float x_distance = fabs(object1.x - object2.x) - ((INIT_PADDLE_SCALE.x + INIT_BALL_SCALE.x) / 2.0f);

    float y_distance = fabs(object1.y - object2.y) - ((INIT_PADDLE_SCALE.y + INIT_BALL_SCALE.y) / 2.0f);
    
    if (y_distance <= 0.15 && x_distance <= -0.4f) {
        return true;
    }
    else {
        return false;
    }
}

int checkScore(glm::vec3 object) {
    if (object.x > 5.0f) {
        return 1;   // player 1 scored
    }
    else if (object.x < -5.0f) {
        return -1;  // player 2 scored
    }
    return 0;       // no score
}

void startGame() {
    p1Upper = false;
    p2Upper = false;
    p1Lower = false;
    p2Lower = false;
    ballLower1 = false;
    ballUpper1 = false;
    ballLower2 = false;
    ballUpper2 = false;
    ballLower3 = false;
    ballUpper3 = false;
    gameProgress = false;
    intialDirection = true;
    twoPlayerMode = true;
    player1Score = false;
    player2Score = false;
    g_playerOne_position = INIT_PLAYERONE_POS;
    g_playerTwo_position = INIT_PLAYERTWO_POS;
    g_ball_position1 = INIT_BALL_POS1;
    g_ball_position2 = INIT_BALL_POS2;
    g_ball_position3 = INIT_BALL_POS3;
    g_ball_movement1.x = 1.0f;
    g_ball_movement2.x = -1.0f;
    g_ball_movement3.x = 1.0f;

}

void drawGameOver() {
    letter_matrix = glm::mat4(1.0f);
    letter_matrix = glm::translate(letter_matrix, glm::vec3(-1.25f, 3.0f, 0.0f));
    letter_matrix = glm::scale(letter_matrix, glm::vec3(0.5f, 0.5f, 0.0f));
    g_shader_program.set_model_matrix(letter_matrix);

    // Draw each letter in "GAME"
    draw_sprite_from_texture_atlas(&g_shader_program, g_keyboard_texture_id, 78, KEYBOARD_DIMENSIONS_ROWS, KEYBOARD_DIMENSIONS_COLS); // G
    letter_matrix = glm::translate(letter_matrix, glm::vec3(0.7f, 0.0f, 0.0f));
    g_shader_program.set_model_matrix(letter_matrix);
    
    draw_sprite_from_texture_atlas(&g_shader_program, g_keyboard_texture_id, 72, KEYBOARD_DIMENSIONS_ROWS, KEYBOARD_DIMENSIONS_COLS); // A
    letter_matrix = glm::translate(letter_matrix, glm::vec3(0.7f, 0.0f, 0.0f));
    g_shader_program.set_model_matrix(letter_matrix);
    
    draw_sprite_from_texture_atlas(&g_shader_program, g_keyboard_texture_id, 84, KEYBOARD_DIMENSIONS_ROWS, KEYBOARD_DIMENSIONS_COLS); // M
    letter_matrix = glm::translate(letter_matrix, glm::vec3(0.7f, 0.0f, 0.0f));
    g_shader_program.set_model_matrix(letter_matrix);

    draw_sprite_from_texture_atlas(&g_shader_program, g_keyboard_texture_id, 76, KEYBOARD_DIMENSIONS_ROWS, KEYBOARD_DIMENSIONS_COLS); // E
    
    // Adjust position add a space
    letter_matrix = glm::translate(letter_matrix, glm::vec3(1.0f, 0.0f, 0.0f));
    g_shader_program.set_model_matrix(letter_matrix);

    // Draw each letter in "OVER"
    draw_sprite_from_texture_atlas(&g_shader_program, g_keyboard_texture_id, 86, KEYBOARD_DIMENSIONS_ROWS, KEYBOARD_DIMENSIONS_COLS); // O
    letter_matrix = glm::translate(letter_matrix, glm::vec3(0.7f, 0.0f, 0.0f));
    g_shader_program.set_model_matrix(letter_matrix);

    draw_sprite_from_texture_atlas(&g_shader_program, g_keyboard_texture_id, 93, KEYBOARD_DIMENSIONS_ROWS, KEYBOARD_DIMENSIONS_COLS); // V
    letter_matrix = glm::translate(letter_matrix, glm::vec3(0.7f, 0.0f, 0.0f));
    g_shader_program.set_model_matrix(letter_matrix);

    draw_sprite_from_texture_atlas(&g_shader_program, g_keyboard_texture_id, 76, KEYBOARD_DIMENSIONS_ROWS, KEYBOARD_DIMENSIONS_COLS); // E
    letter_matrix = glm::translate(letter_matrix, glm::vec3(0.7f, 0.0f, 0.0f));
    g_shader_program.set_model_matrix(letter_matrix);

    draw_sprite_from_texture_atlas(&g_shader_program, g_keyboard_texture_id, 89, KEYBOARD_DIMENSIONS_ROWS, KEYBOARD_DIMENSIONS_COLS); // R
}

void drawSpaceToStart() {
    letter_matrix = glm::mat4(1.0f);
    letter_matrix = glm::translate(letter_matrix, glm::vec3(-1.35f, 2.0f, 0.0f));
    letter_matrix = glm::scale(letter_matrix, glm::vec3(0.5f, 0.5f, 0.0f));
    g_shader_program.set_model_matrix(letter_matrix);

    // Draw "SPACE"
    draw_sprite_from_texture_atlas(&g_shader_program, g_keyboard_extra_texture_id, 26, KEYBOARD_EXTRA_DIMENSIONS_ROWS, KEYBOARD_EXTRA_DIMENSIONS_COLS);
    letter_matrix = glm::translate(letter_matrix, glm::vec3(0.7f, 0.0f, 0.0f));

    letter_matrix = glm::translate(letter_matrix, glm::vec3(0.5f, 0.0f, 0.0f)); // Add extra space
    g_shader_program.set_model_matrix(letter_matrix);

    // Draw "TO"
    draw_sprite_from_texture_atlas(&g_shader_program, g_keyboard_texture_id, 91, KEYBOARD_DIMENSIONS_ROWS, KEYBOARD_DIMENSIONS_COLS); // T
    letter_matrix = glm::translate(letter_matrix, glm::vec3(0.7f, 0.0f, 0.0f));
    g_shader_program.set_model_matrix(letter_matrix);

    draw_sprite_from_texture_atlas(&g_shader_program, g_keyboard_texture_id, 86, KEYBOARD_DIMENSIONS_ROWS, KEYBOARD_DIMENSIONS_COLS); // O
    letter_matrix = glm::translate(letter_matrix, glm::vec3(1.0f, 0.0f, 0.0f)); // Add extra space
    g_shader_program.set_model_matrix(letter_matrix);

    // Draw "START"
    draw_sprite_from_texture_atlas(&g_shader_program, g_keyboard_texture_id, 90, KEYBOARD_DIMENSIONS_ROWS, KEYBOARD_DIMENSIONS_COLS); // S
    letter_matrix = glm::translate(letter_matrix, glm::vec3(0.7f, 0.0f, 0.0f));
    g_shader_program.set_model_matrix(letter_matrix);

    draw_sprite_from_texture_atlas(&g_shader_program, g_keyboard_texture_id, 91, KEYBOARD_DIMENSIONS_ROWS, KEYBOARD_DIMENSIONS_COLS); // T
    letter_matrix = glm::translate(letter_matrix, glm::vec3(0.7f, 0.0f, 0.0f));
    g_shader_program.set_model_matrix(letter_matrix);

    draw_sprite_from_texture_atlas(&g_shader_program, g_keyboard_texture_id, 72, KEYBOARD_DIMENSIONS_ROWS, KEYBOARD_DIMENSIONS_COLS); // A
    letter_matrix = glm::translate(letter_matrix, glm::vec3(0.7f, 0.0f, 0.0f));
    g_shader_program.set_model_matrix(letter_matrix);

    draw_sprite_from_texture_atlas(&g_shader_program, g_keyboard_texture_id, 89, KEYBOARD_DIMENSIONS_ROWS, KEYBOARD_DIMENSIONS_COLS); // R
    letter_matrix = glm::translate(letter_matrix, glm::vec3(0.7f, 0.0f, 0.0f));
    g_shader_program.set_model_matrix(letter_matrix);

    draw_sprite_from_texture_atlas(&g_shader_program, g_keyboard_texture_id, 91, KEYBOARD_DIMENSIONS_ROWS, KEYBOARD_DIMENSIONS_COLS); // T
}

void drawPlayerWins(int playerNumber) {
    glm::mat4 letter_matrix = glm::mat4(1.0f);
    letter_matrix = glm::translate(letter_matrix, glm::vec3(-2.0f, 2.5f, 0.0f));
    letter_matrix = glm::scale(letter_matrix, glm::vec3(0.5f, 0.5f, 0.0f)); // Scale letters
    g_shader_program.set_model_matrix(letter_matrix);

    // Draw "PLAYER"
    draw_sprite_from_texture_atlas(&g_shader_program, g_keyboard_texture_id, 31, KEYBOARD_DIMENSIONS_ROWS, KEYBOARD_DIMENSIONS_COLS); // P
    letter_matrix = glm::translate(letter_matrix, glm::vec3(0.7f, 0.0f, 0.0f));
    g_shader_program.set_model_matrix(letter_matrix);

    draw_sprite_from_texture_atlas(&g_shader_program, g_keyboard_texture_id, 27, KEYBOARD_DIMENSIONS_ROWS, KEYBOARD_DIMENSIONS_COLS); // L
    letter_matrix = glm::translate(letter_matrix, glm::vec3(0.7f, 0.0f, 0.0f));
    g_shader_program.set_model_matrix(letter_matrix);

    draw_sprite_from_texture_atlas(&g_shader_program, g_keyboard_texture_id, 16, KEYBOARD_DIMENSIONS_ROWS, KEYBOARD_DIMENSIONS_COLS); // A
    letter_matrix = glm::translate(letter_matrix, glm::vec3(0.7f, 0.0f, 0.0f));
    g_shader_program.set_model_matrix(letter_matrix);

    draw_sprite_from_texture_atlas(&g_shader_program, g_keyboard_texture_id, 40, KEYBOARD_DIMENSIONS_ROWS, KEYBOARD_DIMENSIONS_COLS); // Y
    letter_matrix = glm::translate(letter_matrix, glm::vec3(0.7f, 0.0f, 0.0f));
    g_shader_program.set_model_matrix(letter_matrix);

    draw_sprite_from_texture_atlas(&g_shader_program, g_keyboard_texture_id, 20, KEYBOARD_DIMENSIONS_ROWS, KEYBOARD_DIMENSIONS_COLS); // E
    letter_matrix = glm::translate(letter_matrix, glm::vec3(0.7f, 0.0f, 0.0f));
    g_shader_program.set_model_matrix(letter_matrix);

    draw_sprite_from_texture_atlas(&g_shader_program, g_keyboard_texture_id, 33, KEYBOARD_DIMENSIONS_ROWS, KEYBOARD_DIMENSIONS_COLS); // R
    g_shader_program.set_model_matrix(letter_matrix);

    letter_matrix = glm::translate(letter_matrix, glm::vec3(1.0f, 0.0f, 0.0f));
    g_shader_program.set_model_matrix(letter_matrix);

    // Draw Player Number
    if (playerNumber == 1) {
        draw_sprite_from_texture_atlas(&g_shader_program, g_keyboard_texture_id, 4, KEYBOARD_DIMENSIONS_ROWS, KEYBOARD_DIMENSIONS_COLS);
    }
    else if (playerNumber == 2) {
        draw_sprite_from_texture_atlas(&g_shader_program, g_keyboard_texture_id, 5, KEYBOARD_DIMENSIONS_ROWS, KEYBOARD_DIMENSIONS_COLS);
    }
    
    letter_matrix = glm::translate(letter_matrix, glm::vec3(1.0f, 0.0f, 0.0f)); // Extra space
    g_shader_program.set_model_matrix(letter_matrix);

    // Draw "WINS"
    draw_sprite_from_texture_atlas(&g_shader_program, g_keyboard_texture_id, 38, KEYBOARD_DIMENSIONS_ROWS, KEYBOARD_DIMENSIONS_COLS); // W
    letter_matrix = glm::translate(letter_matrix, glm::vec3(0.7f, 0.0f, 0.0f));
    g_shader_program.set_model_matrix(letter_matrix);

    draw_sprite_from_texture_atlas(&g_shader_program, g_keyboard_texture_id, 24, KEYBOARD_DIMENSIONS_ROWS, KEYBOARD_DIMENSIONS_COLS); // I
    letter_matrix = glm::translate(letter_matrix, glm::vec3(0.7f, 0.0f, 0.0f));
    g_shader_program.set_model_matrix(letter_matrix);

    draw_sprite_from_texture_atlas(&g_shader_program, g_keyboard_texture_id, 29, KEYBOARD_DIMENSIONS_ROWS, KEYBOARD_DIMENSIONS_COLS); // N
    letter_matrix = glm::translate(letter_matrix, glm::vec3(0.7f, 0.0f, 0.0f));
    g_shader_program.set_model_matrix(letter_matrix);

    draw_sprite_from_texture_atlas(&g_shader_program, g_keyboard_texture_id, 34, KEYBOARD_DIMENSIONS_ROWS, KEYBOARD_DIMENSIONS_COLS); // S
}


void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("Project 2",
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

    if (g_display_window == nullptr)
    {
        std::cerr << "Error: SDL window could not be created.\n";
        shutdown();
    }

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_ball_matrix1   = glm::mat4(1.0f);
    g_ball_matrix2   = glm::mat4(1.0f);
    g_ball_matrix3   = glm::mat4(1.0f);
    g_playerOne_matrix  = glm::mat4(1.0f);
    g_playerTwo_matrix  = glm::mat4(1.0f);
    letter_matrix     = glm::mat4(1.0f);
    g_view_matrix       = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    g_keyboard_texture_id = load_texture(KEYBOARD_SPRITESHEET);
    g_keyboard_extra_texture_id = load_texture(KEYBOARD_EXTRA_SPRITESHEET);
    g_font_texture_id = load_texture(FONTSHEET_FILEPATH);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


void process_input()
{
    g_playerTwo_movement = glm::vec3(0.0f);
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
            case SDL_QUIT:
            case SDL_WINDOWEVENT_CLOSE: g_app_status = TERMINATED; break;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym)
                {
                    case SDLK_q: g_app_status = TERMINATED; break;
                    case SDLK_t:
                        if (gameProgress) {
                            twoPlayerMode = !twoPlayerMode;
                        }
                        break;
                    case SDLK_SPACE:
                        if (!gameProgress) {
                            gameStart = true;
                        }
                        break;
                    case SDLK_1:
                        if (SDLK_1 && !gameProgress) {
                            numBalls = 1;
                        }
                        break;
                    case SDLK_2:
                        if (SDLK_2 && !gameProgress) {
                            numBalls = 2;
                        }
                        break;
                    case SDLK_3:
                        if (SDLK_3 && !gameProgress) {
                            numBalls = 3;
                        }
                        break;
                    default: break;
                }
            default: break;
        }
    }

    const Uint8 *key_state = SDL_GetKeyboardState(NULL);
    
    if (gameProgress) {
        if (twoPlayerMode) { // Player contorl
            g_playerOne_movement = glm::vec3(0.0f);
            intialDirection = true;
            if (key_state[SDL_SCANCODE_W] && !p1Upper)
            {
                g_playerOne_movement.y = 1.0f;
            }
            else if (key_state[SDL_SCANCODE_S] && !p1Lower)
            {
                g_playerOne_movement.y = -1.0f;
            }
        }
        else { //AI contorl
            if (intialDirection) {
                g_playerOne_movement.y = 1.0f;
                intialDirection = false;
            }
            if (p1Lower) {
                g_playerOne_movement.y = 1.0f;
            }
            else if (p1Upper) {
                g_playerOne_movement.y = -1.0f;
            }
        }
        
        if (key_state[SDL_SCANCODE_UP] && !p2Upper)
        {
            g_playerTwo_movement.y = 1.0f;
        }
        else if (key_state[SDL_SCANCODE_DOWN] && !p2Lower)
        {
            g_playerTwo_movement.y = -1.0f;
        }
        
        if (glm::length(g_ball_movement1) > 1.0f)
        {
            g_ball_movement1 = glm::normalize(g_ball_movement1);
        }
        if (glm::length(g_ball_movement2) > 1.0f)
        {
            g_ball_movement1 = glm::normalize(g_ball_movement2);
        }
        if (glm::length(g_ball_movement2) > 1.0f)
        {
            g_ball_movement1 = glm::normalize(g_ball_movement2);
        }
    }
    else { // Stop all current movement
        g_playerOne_movement = glm::vec3(0.0f);
        g_playerTwo_movement = glm::vec3(0.0f);
        g_ball_movement1 = glm::vec3(0.0f);
        g_ball_movement2 = glm::vec3(0.0f);
        g_ball_movement3 = glm::vec3(0.0f);

    }
    
}


void update()
{
    /* DELTA TIME */
    float ticks = (float) SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - previous_ticks;
    previous_ticks = ticks;
    
    if (gameStart) {
        startGame();
        gameStart = false;
        gameProgress = true;
    }
    
    LOG(numBalls);
    /* GAME LOGIC */
    g_ball_position1 += g_ball_movement1 * g_ball_speed * delta_time;
    g_playerTwo_position += g_playerTwo_movement * g_paddle_speed * delta_time;
    g_playerOne_position += g_playerOne_movement * g_paddle_speed * delta_time;
    
    // Check y bounds of objects
    checkYBounds(g_playerOne_position, p1Upper, p1Lower);
    checkYBounds(g_playerTwo_position, p2Upper, p2Lower);
    checkYBounds(g_ball_position1, ballUpper1, ballLower1);
    
    if(ballUpper1) {
        g_ball_movement1.y = -1.0f;
    }
    else if(ballLower1) {
        g_ball_movement1.y = 1.0f;
    }
    
    if (checkScore(g_ball_position1) == -1) {
        player2Score = true;
        gameProgress = false;
    }
    else if(checkScore(g_ball_position1) == 1) {
        player1Score = true;
        gameProgress = false;
    }
    
    
    /* Model matrix reset */
    g_ball_matrix1 = glm::mat4(1.0f);
    g_ball_matrix2 = glm::mat4(1.0f);
    g_ball_matrix3 = glm::mat4(1.0f);
    g_playerOne_matrix = glm::mat4(1.0f);
    g_playerTwo_matrix = glm::mat4(1.0f);
    
    
    /* TRANSFORMATIONS */
    g_ball_matrix1 = glm::translate(g_ball_matrix1, g_ball_position1);
    g_playerOne_matrix = glm::translate(g_playerOne_matrix, g_playerOne_position);
    g_playerTwo_matrix = glm::translate(g_playerTwo_matrix, g_playerTwo_position);
    g_playerOne_matrix = glm::rotate(g_playerOne_matrix, glm::radians(-90.0f), glm::vec3(0.0f,0.0f, 1.0f));
    g_playerTwo_matrix = glm::rotate(g_playerTwo_matrix, glm::radians(90.0f), glm::vec3(0.0f,0.0f, 1.0f));
    g_ball_matrix1 = glm::scale(g_ball_matrix1, INIT_BALL_SCALE);
    g_playerTwo_matrix = glm::scale(g_playerTwo_matrix, INIT_PADDLE_SCALE);
    g_playerOne_matrix = glm::scale(g_playerOne_matrix, INIT_PADDLE_SCALE);
    
    
    if (numBalls == 2) {
        g_ball_position2 += g_ball_movement2 * (g_ball_speed * 0.75f) * delta_time;
        g_ball_matrix2 = glm::translate(g_ball_matrix2, g_ball_position2);
        g_ball_matrix2 = glm::translate(g_ball_matrix2, glm::vec3(0.0f, 0.25f, 0.0f));
        g_ball_matrix2 = glm::scale(g_ball_matrix2, INIT_BALL_SCALE);
        if (checkScore(g_ball_position2) == -1) {
            player2Score = true;
            gameProgress = false;
        }
        else if(checkScore(g_ball_position2) == 1) {
            player1Score = true;
            gameProgress = false;
        }
    }
    else if(numBalls == 3) {
        g_ball_position2 += g_ball_movement2 * (g_ball_speed * 0.01f) * delta_time;
        checkYBounds(g_ball_position2, ballUpper2, ballLower2);
        if (checkScore(g_ball_position2) == -1) {
            player2Score = true;
            gameProgress = false;
        }
        else if(checkScore(g_ball_position2) == 1) {
            player1Score = true;
            gameProgress = false;
        }
        g_ball_matrix2 = glm::translate(g_ball_matrix2, g_ball_position2);
        g_ball_matrix2 = glm::translate(g_ball_matrix2, glm::vec3(0.0f, 0.25f, 0.0f));
        g_ball_matrix2 = glm::scale(g_ball_matrix2, INIT_BALL_SCALE);
        
        g_ball_position3 += g_ball_movement3 * (g_ball_speed * 0.01f) * delta_time;
        checkYBounds(g_ball_position3, ballUpper3, ballLower3);
        if (checkScore(g_ball_position3) == -1) {
            player2Score = true;
            gameProgress = false;
        }
        else if(checkScore(g_ball_position3) == 1) {
            player1Score = true;
            gameProgress = false;
        }
        g_ball_matrix3 = glm::translate(g_ball_matrix3, g_ball_position3);
        g_ball_matrix3 = glm::translate(g_ball_matrix3, glm::vec3(0.0f, -0.25f, 0.0f));
        g_ball_matrix3 = glm::scale(g_ball_matrix3, INIT_BALL_SCALE);
    }
    
    if (checkCollision(g_playerOne_position, g_ball_position1)) {
        g_ball_movement1.x = 1.0f;
        g_ball_movement1.y = 1.0f;
        
    }
    if (checkCollision(g_playerTwo_position, g_ball_position1)) {
        g_ball_movement1.x = -1.0f;
        g_ball_movement1.y = -1.0f;
    }
    
    if (numBalls == 2 ){
        if (checkCollision(g_playerOne_position, g_ball_position2)) {
            g_ball_movement2.x = 1.0f;
            g_ball_movement2.y = 1.0f;
            
        }
        if (checkCollision(g_playerTwo_position, g_ball_position2)) {
            g_ball_movement2.x = -1.0f;
            g_ball_movement2.y = -1.0f;
        }
    }
    else if (numBalls == 3) {
        if (checkCollision(g_playerOne_position, g_ball_position2)) {
            g_ball_movement2.x = 1.0f;
            g_ball_movement2.y = 1.0f;
            
        }
        if (checkCollision(g_playerTwo_position, g_ball_position2)) {
            g_ball_movement2.x = -1.0f;
            g_ball_movement2.y = -1.0f;
        }
        
        if (checkCollision(g_playerOne_position, g_ball_position3)) {
            g_ball_movement3.x = 1.0f;
            g_ball_movement3.y = 1.0f;
            
        }
        if (checkCollision(g_playerTwo_position, g_ball_position3)) {
            g_ball_movement3.x = -1.0f;
            g_ball_movement3.y = -1.0f;
        }
    }
}


void draw_object(glm::mat4 &object_model_matrix, GLuint &object_texture_id)
{
    g_shader_program.set_model_matrix(object_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}


void render()
{
    glClear(GL_COLOR_BUFFER_BIT);

    g_shader_program.set_model_matrix(g_ball_matrix1);
    draw_sprite_from_texture_atlas(&g_shader_program, g_keyboard_texture_id,
                                   42,
                                   KEYBOARD_DIMENSIONS_ROWS, KEYBOARD_DIMENSIONS_COLS);
    
    
    g_shader_program.set_model_matrix(g_playerOne_matrix);
    draw_sprite_from_texture_atlas(&g_shader_program, g_keyboard_extra_texture_id,
                                   4,
                                   KEYBOARD_EXTRA_DIMENSIONS_ROWS,  KEYBOARD_EXTRA_DIMENSIONS_COLS);
    
    g_shader_program.set_model_matrix(g_playerTwo_matrix);
    draw_sprite_from_texture_atlas(&g_shader_program, g_keyboard_extra_texture_id,
                                   20,
                                   KEYBOARD_EXTRA_DIMENSIONS_ROWS,  KEYBOARD_EXTRA_DIMENSIONS_COLS);
    if (numBalls == 2) {
        g_shader_program.set_model_matrix(g_ball_matrix2);
        draw_sprite_from_texture_atlas(&g_shader_program, g_keyboard_texture_id,
                                       42,
                                       KEYBOARD_DIMENSIONS_ROWS, KEYBOARD_DIMENSIONS_COLS);
    }
    else if (numBalls == 3) {
        g_shader_program.set_model_matrix(g_ball_matrix2);
        draw_sprite_from_texture_atlas(&g_shader_program, g_keyboard_texture_id,
                                       42,
                                       KEYBOARD_DIMENSIONS_ROWS, KEYBOARD_DIMENSIONS_COLS);
        g_shader_program.set_model_matrix(g_ball_matrix3);
        draw_sprite_from_texture_atlas(&g_shader_program, g_keyboard_texture_id,
                                       42,
                                       KEYBOARD_DIMENSIONS_ROWS, KEYBOARD_DIMENSIONS_COLS);
    }

    if (player1Score) {
        drawGameOver();
        drawSpaceToStart();
        drawPlayerWins(1);
    }
    else if (player2Score) {
        drawGameOver();
        drawSpaceToStart();
        drawPlayerWins(2);
    }
    
    if (!gameProgress) {
        drawSpaceToStart();
    }
    

    SDL_GL_SwapWindow(g_display_window);
}


void shutdown() { SDL_Quit(); }


int main(int argc, char* argv[])
{
    initialise();

    while (g_app_status == RUNNING)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}
