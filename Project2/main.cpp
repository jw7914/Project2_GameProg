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

/* -------------------------- NEW STUFF BELOW -------------------------- */
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
     ballLower = false,
     ballUpper = false,
     gameStart = false,
     gameProgress = false,
     intialDirection = true,
     twoPlayerMode = true,
     player1Score = false,
     player2Score = false;

void draw_sprite_from_texture_atlas(ShaderProgram *program, GLuint texture_id, int index,
                                    int rows, int cols);
void draw_text(ShaderProgram *shader_program, GLuint font_texture_id, std::string text,
               float font_size, float spacing, glm::vec3 position);
/* -------------------------- NEW STUFF ABOVE -------------------------- */

SDL_Window* g_display_window = nullptr;
AppStatus g_app_status = RUNNING;

ShaderProgram g_shader_program = ShaderProgram();

glm::mat4 g_view_matrix,
          g_ball_matrix,
          g_playerOne_matrix,
          g_playerTwo_matrix,
          g_projection_matrix;

float previous_ticks = 0.0f;

glm::vec3 INIT_PADDLE_SCALE = glm::vec3(1.0f, 0.25f, 0.0f);
glm::vec3 INIT_BALL_SCALE = glm::vec3(0.3f, 0.3f, 0.0f);
glm::vec3 INIT_PLAYERONE_POS = glm::vec3(-4.5f, 0.0f, 0.0f);
glm::vec3 INIT_PLAYERTWO_POS = glm::vec3(4.5f, 0.0f, 0.0f);
glm::vec3 INIT_BALL_POS = glm::vec3(0.0f, 0.0f, 0.0f);

glm::vec3 g_ball_position = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_playerOne_position = glm::vec3(-4.5f, 0.0f, 0.0f);
glm::vec3 g_playerTwo_position = glm::vec3(4.5f, 0.0f, 0.0f);

glm::vec3 g_ball_movement = glm::vec3(0.0f, 0.0f, 0.0f);
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


void draw_text(ShaderProgram *shader_program, GLuint font_texture_id, std::string text,
               float font_size, float spacing, glm::vec3 position)
{
    // Scale the size of the fontbank in the UV-plane
    // We will use this for spacing and positioning
    float width = 1.0f / FONTBANK_SIZE;
    float height = 1.0f / FONTBANK_SIZE;

    // Instead of having a single pair of arrays, we'll have a series of pairs—one for
    // each character. Don't forget to include <vector>!
    std::vector<float> vertices;
    std::vector<float> texture_coordinates;

    // For every character...
    for (int i = 0; i < text.size(); i++) {
        // 1. Get their index in the spritesheet, as well as their offset (i.e. their
        //    position relative to the whole sentence)
        int spritesheet_index = (int) text[i];  // ascii value of character
        float offset = (font_size + spacing) * i;

        // 2. Using the spritesheet index, we can calculate our U- and V-coordinates
        float u_coordinate = (float) (spritesheet_index % FONTBANK_SIZE) / FONTBANK_SIZE;
        float v_coordinate = (float) (spritesheet_index / FONTBANK_SIZE) / FONTBANK_SIZE;

        // 3. Inset the current pair in both vectors
        vertices.insert(vertices.end(), {
            offset + (-0.5f * font_size), 0.5f * font_size,
            offset + (-0.5f * font_size), -0.5f * font_size,
            offset + (0.5f * font_size), 0.5f * font_size,
            offset + (0.5f * font_size), -0.5f * font_size,
            offset + (0.5f * font_size), 0.5f * font_size,
            offset + (-0.5f * font_size), -0.5f * font_size,
        });

        texture_coordinates.insert(texture_coordinates.end(), {
            u_coordinate, v_coordinate,
            u_coordinate, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate + width, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate, v_coordinate + height,
        });
    }

    // 4. And render all of them using the pairs
    glm::mat4 model_matrix = glm::mat4(1.0f);
    model_matrix = glm::translate(model_matrix, position);

    shader_program->set_model_matrix(model_matrix);
    glUseProgram(shader_program->get_program_id());

    glVertexAttribPointer(shader_program->get_position_attribute(), 2, GL_FLOAT, false, 0,
                          vertices.data());
    glEnableVertexAttribArray(shader_program->get_position_attribute());

    glVertexAttribPointer(shader_program->get_tex_coordinate_attribute(), 2, GL_FLOAT,
                          false, 0, texture_coordinates.data());
    glEnableVertexAttribArray(shader_program->get_tex_coordinate_attribute());

    glBindTexture(GL_TEXTURE_2D, font_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, (int) (text.size() * 6));

    glDisableVertexAttribArray(shader_program->get_position_attribute());
    glDisableVertexAttribArray(shader_program->get_tex_coordinate_attribute());
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
    ballLower = false;
    ballUpper = false;
    gameProgress = false;
    intialDirection = true;
    twoPlayerMode = true;
    player1Score = false;
    player2Score = false;
    g_playerOne_position = INIT_PLAYERONE_POS;
    g_playerTwo_position = INIT_PLAYERTWO_POS;
    g_ball_position = INIT_BALL_POS;
    g_ball_movement.x = 1.0f;
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

    g_ball_matrix   = glm::mat4(1.0f);
    g_playerOne_matrix  = glm::mat4(1.0f);
    g_playerTwo_matrix  = glm::mat4(1.0f);
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
        
        if (key_state[SDL_SCANCODE_LEFT])
        {
            g_ball_movement.x = -1.0f;
        }
        else if (key_state[SDL_SCANCODE_RIGHT]) {
            g_ball_movement.x = 1.0f;
        }

        if (glm::length(g_ball_movement) > 1.0f)
        {
            g_ball_movement = glm::normalize(g_ball_movement);
        }
    }
    else { // Stop all current movement
        g_playerOne_movement = glm::vec3(0.0f);
        g_playerTwo_movement = glm::vec3(0.0f);
        g_ball_movement = glm::vec3(0.0f);
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
    
    /* GAME LOGIC */
    g_ball_position += g_ball_movement * g_ball_speed * delta_time;
    g_playerTwo_position += g_playerTwo_movement * g_paddle_speed * delta_time;
    g_playerOne_position += g_playerOne_movement * g_paddle_speed * delta_time;
    
    // Check y bounds of objects
    checkYBounds(g_playerOne_position, p1Upper, p1Lower);
    checkYBounds(g_playerTwo_position, p2Upper, p2Lower);
    checkYBounds(g_ball_position, ballUpper, ballLower);
    
    if(ballUpper) {
        g_ball_movement.y = -1.0f;
    }
    else if(ballLower) {
        g_ball_movement.y = 1.0f;
    }
    
    if (checkScore(g_ball_position) == -1) {
        player2Score = true;
        gameProgress = false;
    }
    else if(checkScore(g_ball_position) == 1) {
        player1Score = true;
        gameProgress = false;
    }
    
    
    /* Model matrix reset */
    g_ball_matrix = glm::mat4(1.0f);
    g_playerOne_matrix = glm::mat4(1.0f);
    g_playerTwo_matrix = glm::mat4(1.0f);
    

    /* TRANSFORMATIONS */
    g_ball_matrix = glm::translate(g_ball_matrix, g_ball_position);
    g_playerOne_matrix = glm::translate(g_playerOne_matrix, g_playerOne_position);
    g_playerTwo_matrix = glm::translate(g_playerTwo_matrix, g_playerTwo_position);
    g_playerOne_matrix = glm::rotate(g_playerOne_matrix, glm::radians(-90.0f), glm::vec3(0.0f,0.0f, 1.0f));
    g_playerTwo_matrix = glm::rotate(g_playerTwo_matrix, glm::radians(90.0f), glm::vec3(0.0f,0.0f, 1.0f));
    g_ball_matrix = glm::scale(g_ball_matrix, INIT_BALL_SCALE);
    g_playerTwo_matrix = glm::scale(g_playerTwo_matrix, INIT_PADDLE_SCALE);
    g_playerOne_matrix = glm::scale(g_playerOne_matrix, INIT_PADDLE_SCALE);
    
    if (checkCollision(g_playerOne_position, g_ball_position)) {
        g_ball_movement.x = 1.0f;
        g_ball_movement.y = 1.0f;
        
    }
    if (checkCollision(g_playerTwo_position, g_ball_position)) {
        g_ball_movement.x = -1.0f;
        g_ball_movement.y = -1.0f;
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

    g_shader_program.set_model_matrix(g_ball_matrix);
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
    if (player1Score) {
        draw_text(&g_shader_program, g_font_texture_id, "Player One Wins", 0.5f, 0.05f,
                  glm::vec3(-3.0f, 3.0f, 0.0f));
        draw_text(&g_shader_program, g_font_texture_id, "Press Space To Replay", 0.5f, 0.05f,
                  glm::vec3(-3.0f, 1.0f, 0.0f));
    }
    else if (player2Score) {
        draw_text(&g_shader_program, g_font_texture_id, "Player Two Wins", 0.5f, 0.05f,
                  glm::vec3(-3.0f, 3.0f, 0.0f));
        draw_text(&g_shader_program, g_font_texture_id, "Press Space To Replay", 0.5f, 0.05f,
                  glm::vec3(-3.0f, 1.0f, 0.0f));
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
