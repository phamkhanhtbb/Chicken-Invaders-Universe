#include "game_management.h"

// Constructor - Initializes game variables with default values
game_management::game_management(){
    isRunning = true;                        // Game is running by default
    bullet_level = 0;                        // Start with base bullet level
    kill = 0;                                // Initialize kill counter
    scrolling = -(BACKGROUND_HEIGHT - SCREEN_HEIGHT);  // Initialize background scrolling position
    time_end_game = 0;                       // Counter for end game screen duration
    count_num_exp = 0;                       // Explosion animation counter
    menu_number = 0;                         // Menu selection

    isPaused = false;                        // Game starts unpaused
    pauseButtonState = 0;                    // 0 = play state, 1 = pause state
    current_music = nullptr;                 // No music playing initially
    pause_overlay_texture = nullptr;         // Pause overlay not created yet
}

// Destructor
game_management::~game_management(){
    // Empty destructor - cleanup handled in clean() method
}

// Initialize the game components, window, renderer, and resources
void game_management::init(std::string title){
    // Initialize SDL Video
    if(SDL_Init(SDL_INIT_VIDEO) < 0){
        isRunning = false;                   // If SDL initialization fails, don't run the game
    }
    else{
        // Create game window
        gWindow = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                  SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
        if(gWindow == NULL){
            isRunning = false;               // If window creation fails, don't run the game
        }
        else{
            // Set render quality hint
            SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");

            // Create renderer
            gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
            if(gRenderer == NULL){
                isRunning = false;           // If renderer creation fails, don't run the game
            }
            else{
                // Set default render draw color
                SDL_SetRenderDrawColor(gRenderer, RENDER_DRAW_COLOR, RENDER_DRAW_COLOR, RENDER_DRAW_COLOR, RENDER_DRAW_COLOR);

                // Initialize SDL_image for PNG loading
                int imgFlags = IMG_INIT_PNG;
                if(!(IMG_Init(imgFlags) & imgFlags)){
                    isRunning = false;       // If image initialization fails, don't run the game
                }

                // Initialize SDL_mixer for audio
                if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0){
                    isRunning = false;       // If audio initialization fails, don't run the game
                }

                // Load sound effects
                g_sound_bullet[0] = Mix_LoadWAV("sound//blaster.wav");     // Blaster sound
                g_sound_bullet[1] = Mix_LoadWAV("sound//neutron.wav");     // Neutron sound
                g_sound_bullet[2] = Mix_LoadWAV("sound//boron.wav");       // Boron sound
                g_sound_exp[0] = Mix_LoadWAV("sound//exp.wav");            // Explosion sound
                g_sound_exp[1] = Mix_LoadWAV("sound//exp_uco.wav");        // Alternative explosion sound
                g_sound_chicken_hit[0] = Mix_LoadWAV("sound//ChickenHit.wav");  // Enemy hit sound 1
                g_sound_chicken_hit[1] = Mix_LoadWAV("sound//ChickenHit2.wav"); // Enemy hit sound 2
                g_sound_chicken_hit[2] = Mix_LoadWAV("sound//ChickenHit3.wav"); // Enemy hit sound 3
                g_sound_level_up = Mix_LoadWAV("sound//level_up.wav");     // Level up sound
                g_music_start = Mix_LoadMUS("sound//start.mp3");           // Start/menu music

                // Check if all sounds loaded successfully
                if(g_sound_bullet[0] == NULL
                || g_sound_bullet[1] == NULL
                || g_sound_bullet[2] == NULL
                || g_sound_exp[0] == NULL
                || g_sound_exp[1] == NULL
                || g_sound_chicken_hit[0] == NULL
                || g_sound_chicken_hit[1] == NULL
                || g_sound_chicken_hit[2] == NULL
                || g_sound_level_up == NULL
                || g_music_start == NULL){
                    isRunning = false;       // If any sound failed to load, don't run the game
                }

                // Initialize SDL_ttf for font rendering
                if(TTF_Init() < 0){
                    isRunning = false;       // If font initialization fails, don't run the game
                }

                // Load fonts in different sizes
                g_font_text = TTF_OpenFont("font//font1.ttf", 24);         // Regular text font
                g_font_menu = TTF_OpenFont("font//font1.ttf", 50);         // Menu font
                g_font_end_game = TTF_OpenFont("font//font1.ttf", 150);    // Game over/win font

                if(g_font_text == NULL || g_font_menu == NULL || g_font_end_game == NULL){
                    isRunning = false;       // If any font failed to load, don't run the game
                }
            }
        }
    }

    // Initialize background and player's spaceship
    background.loadImg("image//background.png", gRenderer);
    spaceship.loadImg("image//spacecraft.png", gRenderer);

    // Initialize boss enemy
    boss.loadImg("image//boss.png", gRenderer);
    boss.set_clips();                        // Set animation frame clips
    boss.SetRect(-WIDTH_BOSS, -HEIGHT_BOSS); // Position boss off-screen initially
    boss.set_y_val(BOSS_SPEED);              // Set vertical movement speed
    boss.set_x_val(BOSS_SPEED * 3);          // Set horizontal movement speed (faster than vertical)
    boss.set_heart(BOSS_HEART);              // Set boss health

    // Initialize boss bullets
    bullet* p_bullet = new bullet();
    boss.InitBullet(p_bullet, gRenderer);

    // Initialize chicken enemies
    int t = 0;                              // Counter for horizontal positioning
    int y_row = 0;                          // Starting vertical position
    for(int c = 0; c < NUMBER_OF_CHICKEN; c++){
        Chicken* p_chicken = new Chicken();
        if(p_chicken){
            p_chicken->loadImg("image//chicken_red.png", gRenderer);
            p_chicken->set_clips();          // Set animation frame clips

            // Position chickens in rows
            if(t % NUMBER_OF_CHICKENS_PER_ROW == 0){
                y_row -= DISTANCE_BETWEEN_CHICKENS;
                t = 0;
            }

            p_chicken->SetRect(10 + t * DISTANCE_BETWEEN_CHICKENS, y_row);
            p_chicken->set_y_val(CHICKEN_SPEED);     // Set vertical movement speed
            p_chicken->set_x_val(CHICKEN_SPEED);     // Set horizontal movement speed
            p_chicken->set_heart(CHICKEN_HEART);     // Set chicken health

            // Randomly give some chickens the ability to shoot
            int random = rand() % 8;
            if(random < 2){                  // ~25% chance to be able to shoot
                bullet* p_bullet = new bullet();
                p_chicken->InitBullet(p_bullet, gRenderer);
            }

            p_chicken_list.push_back(p_chicken);
            t++;
        }
    }

    // Initialize explosion animations
    exp.loadImg("image//exp.png", gRenderer);
    exp.set_clip();                         // Set explosion animation frames
    exp_boss.loadImg("image//exp.png", gRenderer);
    exp_boss.set_clip();                    // Set boss explosion animation frames
    gift.set_clip();                        // Set power-up animation frames

    // Initialize support UI
    support.loadImg("image//support.png", gRenderer);
    support.SetRect(-20, 10);               // Position support UI

    // Set text colors for UI elements
    kill_text.SetColor(Text::WHITE);        // Kill counter text
    heart_text.SetColor(Text::WHITE);       // Health counter text
    lighting_text.SetColor(Text::WHITE);    // Bullet level text
    hint.SetColor(Text::WHITE);             // Hint text
    end_game.SetColor(Text::WHITE);         // End game message text

    isRunning = true;                       // Game is ready to run

    // Set up pause button position and size
    pauseButtonRect.x = SCREEN_WIDTH - 70;  // Top-right corner
    pauseButtonRect.y = 10;
    pauseButtonRect.w = 50;
    pauseButtonRect.h = 50;

    // Load pause button image (starts in play state)
    pauseButton.loadImg("image//play.png", gRenderer);

    // Create pause overlay texture
    pause_overlay_texture = SDL_CreateTexture(
        gRenderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        SCREEN_WIDTH,
        SCREEN_HEIGHT
    );
    SDL_SetTextureBlendMode(pause_overlay_texture, SDL_BLENDMODE_BLEND);
}

// Create the pause overlay with semi-transparent background and text
void game_management::render_pause_overlay() {
    SDL_SetRenderTarget(gRenderer, pause_overlay_texture);

    // Create semi-transparent black background
    SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 128);
    SDL_RenderClear(gRenderer);

    // Create and position "GAME PAUSED" text
    Text pauseText;
    pauseText.SetText("GAME PAUSED");
    pauseText.SetColor(Text::WHITE);

    // Calculate text dimensions for centering
    int text_width = 0, text_height = 0;
    TTF_SizeText(g_font_menu, "GAME PAUSED", &text_width, &text_height);

    // Center the text on screen
    pauseText.SetRect(
        (SCREEN_WIDTH - text_width) / 2,
        (SCREEN_HEIGHT - text_height) / 2
    );
    pauseText.loadText_showText(g_font_menu, gRenderer);

    // Reset render target back to the window
    SDL_SetRenderTarget(gRenderer, NULL);

    // Copy the overlay to the renderer
    SDL_RenderCopy(gRenderer, pause_overlay_texture, NULL, NULL);
}

// Pause the game
void game_management::pause_game() {
    if (isPaused) return;  // Already paused, do nothing

    isPaused = true;
    pauseButtonState = 1;  // Change button to "pause" state

    // Pause all audio
    Mix_PauseMusic();
    for (int i = 0; i < MIX_CHANNELS; ++i) {
        if (Mix_Playing(i)) {
            Mix_Pause(i);
        }
    }

    // Create and show the pause overlay
    render_pause_overlay();

    // Play pause sound
    Mix_PlayChannel(-1, g_sound_level_up, 0);
}

// Resume the game
void game_management::resume_game() {
    if (!isPaused) return;  // Already running, do nothing

    isPaused = false;
    pauseButtonState = 0;  // Change button to "play" state

    // Resume background music if it was playing
    if (current_music) {
        Mix_ResumeMusic();
    }

    // Resume sound effects
    for (auto& sound : active_sounds) {
        Mix_Resume(Mix_PlayChannel(-1, sound, 0));
    }
    active_sounds.clear();

    // Play resume sound
    Mix_PlayChannel(-1, g_sound_level_up, 0);
}

// Check for collision between two rectangles
bool game_management::check_collision(const SDL_Rect& object1, const SDL_Rect& object2){
    // Extract rectangle boundaries
    int left_a = object1.x;
    int right_a = object1.x + object1.w;
    int top_a = object1.y;
    int bottom_a = object1.y + object1.h;

    int left_b = object2.x;
    int right_b = object2.x + object2.w;
    int top_b = object2.y;
    int bottom_b = object2.y + object2.h;

    // Check if rectangles do NOT overlap (using reverse logic)
    if(left_a > right_b || right_a < left_b || top_a > bottom_b || bottom_a < top_b){
        return false;
    }
    return true;  // Rectangles overlap
}

// Check if mouse coordinates are within a rectangle (for menu/button clicks)
bool game_management::check_mouse_vs_item(const int& x, const int& y, const SDL_Rect& rect){
    if(x >= rect.x && x <= rect.x + rect.w && y >= rect.y && y <= rect.y + rect.h){
        return true;  // Mouse is inside rectangle
    }
    return false;     // Mouse is outside rectangle
}

// Display and handle the game menu
void game_management::menu(const std::string& item) {
    base menu;

    // Load menu background
    if(!menu.loadImg("image//menu.png", gRenderer)) {
        isRunning = false;
        return;
    }

    // Define menu options
    const int number_of_item = 2;
    SDL_Rect pos_arr[number_of_item];
    Text text_menu[number_of_item];

    // Play/Play Again option
    text_menu[0].SetText(item);
    text_menu[0].SetColor(Text::BLACK);
    text_menu[0].loadText_showText(g_font_menu, gRenderer);
    pos_arr[0].x = SCREEN_WIDTH/2 - text_menu[0].GetRect().w/2;  // Center horizontally
    pos_arr[0].y = SCREEN_HEIGHT - 250;
    text_menu[0].SetRect(pos_arr[0].x, pos_arr[0].y);

    // Quit option
    text_menu[1].SetText("Quit !");
    text_menu[1].SetColor(Text::BLACK);
    text_menu[1].loadText_showText(g_font_menu, gRenderer);
    pos_arr[1].x = SCREEN_WIDTH/2 - text_menu[1].GetRect().w/2;  // Center horizontally
    pos_arr[1].y = SCREEN_HEIGHT - 150;
    text_menu[1].SetRect(pos_arr[1].x, pos_arr[1].y);

    int xm = 0;  // Mouse x coordinate
    int ym = 0;  // Mouse y coordinate

    // Play menu music
    Mix_PlayMusic(g_music_start, -1);
    bool quit = true;

    // Menu loop
    while(quit) {
        // Render menu background
        menu.Render(gRenderer);

        // Render menu text
        for(int i = 0; i < number_of_item; i++) {
            text_menu[i].loadText_showText(g_font_menu, gRenderer);
        }

        // Handle events
        while(SDL_PollEvent(&gEvent) != 0) {
            // Process spaceship control events (for visual effect in menu)
            spaceship.Control(gEvent, gRenderer, g_sound_bullet, bullet_level, g_sound_level_up);

            if(gEvent.type == SDL_QUIT) {
                isRunning = false;  // Exit the game
                quit = false;       // Exit the menu
            }
            else if(gEvent.type == SDL_MOUSEMOTION) {
                // Track mouse movement
                xm = gEvent.motion.x;
                ym = gEvent.motion.y;

                // Update menu item colors on hover
                for(int i = 0; i < number_of_item; i++) {
                    if(check_mouse_vs_item(xm, ym, text_menu[i].GetRect())) {
                        text_menu[i].SetColor(Text::WHITE);  // Highlight on hover
                    }
                    else {
                        text_menu[i].SetColor(Text::BLACK);  // Normal color
                    }
                }
            }

            if(gEvent.type == SDL_MOUSEBUTTONDOWN) {
                // Track mouse click position
                xm = gEvent.button.x;
                ym = gEvent.button.y;

                // Handle menu item clicks
                for(int i = 0; i < number_of_item; i++) {
                    if(check_mouse_vs_item(xm, ym, text_menu[i].GetRect())) {
                        if(i == 0) {  // Play/Play Again option
                            isRunning = true;
                            quit = false;
                        }
                        else if(i == 1) {  // Quit option
                            isRunning = false;
                            quit = false;
                        }
                        // Play selection sound
                        Mix_PlayChannel(-1, g_sound_level_up, 0);
                    }
                }
            }
        }
        // Update the screen
        SDL_RenderPresent(gRenderer);
    }
    // Stop menu music when exiting menu
    Mix_PauseMusic();
}

// Reset the game state for a new game
void game_management::reset_game(){
    // Reset game variables
    bullet_level = 0;
    kill = 0;
    count_num_exp = 0;
    scrolling = -(BACKGROUND_HEIGHT - SCREEN_HEIGHT);
    time_end_game = 0;

    // Reset player spacecraft
    spaceship.set_bul_type(BLASTER);  // Reset to basic weapon
    spaceship.SetRect(SCREEN_WIDTH/2, SCREEN_HEIGHT - HEIGHT_MAIN);  // Reset position
    spaceship.set_status(true);       // Set to active
    spaceship.set_heart(CHICKEN_HEART);  // Reset health

    // Reset boss
    boss.SetRect(-WIDTH_BOSS, -HEIGHT_BOSS);  // Move off-screen
    boss.set_heart(BOSS_HEART);               // Reset health

    // Reset boss bullets
    for(int i = 0; i < boss.get_bullet_list().size(); i++){
        bullet* p_bullet = boss.get_bullet_list().at(i);
        if(p_bullet){
            p_bullet->SetRect(boss.GetRect().x + WIDTH_BOSS / 2, boss.GetRect().y + HEIGHT_BOSS);
        }
    }

    // Reset chicken enemies
    int t = 0;
    int y_row = -DISTANCE_BETWEEN_CHICKENS;
    for(int c = 0; c < NUMBER_OF_CHICKEN; c++){
        Chicken* p_chicken = p_chicken_list.at(c);
        if(t % NUMBER_OF_CHICKENS_PER_ROW == 0){
            y_row -= DISTANCE_BETWEEN_CHICKENS;
            t = 0;
        }
        p_chicken->set_come_back(true);  // Enable enemies to come back
        p_chicken->SetRect(10 + t * DISTANCE_BETWEEN_CHICKENS, y_row);  // Reset position
        p_chicken->set_heart(CHICKEN_HEART);  // Reset health
        p_chicken->set_status_right();  // Set initial movement direction
        t++;

        // Reset chicken bullets
        for(int i = 0; i < p_chicken->get_bullet_list().size(); i++){
            bullet* p_bullet = p_chicken->get_bullet_list().at(i);
            if(p_bullet){
                p_bullet->SetRect(p_chicken->GetRect().x + WIDTH_CHICKEN / 2 - p_bullet->GetRect().w / 2,
                                 p_chicken->GetRect().y + HEIGHT_CHICKEN);
            }
        }
    }
}

// Handle user input events
void game_management::handle_event() {
    while(SDL_PollEvent(&gEvent)) {
        if(gEvent.type == SDL_QUIT) {
            isRunning = false;  // Exit the game if window close button is clicked
        }

        // Handle pause button clicks
        if(gEvent.type == SDL_MOUSEBUTTONDOWN &&
           gEvent.button.button == SDL_BUTTON_LEFT) {
            int x = gEvent.button.x;
            int y = gEvent.button.y;

            // Check if pause button is clicked
            if(check_mouse_vs_item(x, y, pauseButtonRect)) {
                if (isPaused) {
                    resume_game();
                } else {
                    pause_game();
                }
            }
        }

        // Handle ESC key for pause functionality
        if(gEvent.type == SDL_KEYDOWN) {
            if(gEvent.key.keysym.sym == SDLK_ESCAPE) {
                if (isPaused) {
                    resume_game();
                } else {
                    pause_game();
                }
            }
        }

        // Only allow spaceship control when game is not paused
        if(!isPaused) {
            spaceship.Control(
                gEvent,
                gRenderer,
                g_sound_bullet,
                bullet_level,
                g_sound_level_up
            );
        }
    }
}

// Handle boss enemy logic and interactions
void game_management::handle_boss(){
    // Only show boss after defeating enough enemies and if boss still has health
    if(kill >= NUMBER_OF_CHICKEN*2 && boss.get_heart() >= 0){
        // Display boss health bar
        boss.show_heart_boss(gRenderer, 420, 20, boss.get_heart(), 6);

        // Update boss position and rendering
        boss.Move();
        boss.Show(gRenderer);
        boss.MakeBullet(gRenderer);

        // Check collisions between boss bullets and player
        bool Col1 = false;
        std::vector<bullet*> boss_bullet_list = boss.get_bullet_list();
        for(int b = 0; b < boss_bullet_list.size(); b++){
            bullet* p_bullet = boss_bullet_list.at(b);
            if(p_bullet){
                Col1 = check_collision(p_bullet->GetRect(), spaceship.GetRect());
            }
        }

        // Check collision between boss and player
        bool Col2 = check_collision(spaceship.GetRect(), boss.GetRectFrame());

        // Handle player damage from boss or boss bullets
        if(Col1 || Col2){
            // Play explosion sound
            Mix_PlayChannel(-1, g_sound_exp[0], 0);

            // Create explosion animation at player position
            int x_pos = (spaceship.GetRect().x + WIDTH_MAIN / 2) - WIDTH_FRAME_EXP / 2;
            int y_pos = (spaceship.GetRect().y + HEIGHT_MAIN / 2) - HEIGHT_FRAME_EXP / 2;
            exp.SetRect(x_pos, y_pos);
            exp.set_frame(0);

            // Move player off-screen temporarily
            spaceship.SetRect(SCREEN_WIDTH*2, SCREEN_HEIGHT *2);
            spaceship.set_status(false);
            spaceship.decrease_heart();

            // Reduce weapon level when player is hit
            if(spaceship.get_heart() >= 0){
                bullet_level = bullet_level < 2 ? 0 : (bullet_level - 1);
            }
        }

        // Check collisions between player bullets and boss
        std::vector<bullet*> s_bullet_list = spaceship.get_bullet_list();
        for(int sb = 0; sb < s_bullet_list.size(); sb++){
            bullet* p_bullet = s_bullet_list.at(sb);
            if(p_bullet != NULL){
                bool Col3 = check_collision(p_bullet->GetRect(), boss.GetRectFrame());
                if(Col3){
                    // Deal damage to boss based on bullet level
                    boss.Decrease((spaceship.get_bullet_damage()) + bullet_level * BULLET_DAMAGE_LEVEL_UP);
                    // Play hit sound
                    Mix_PlayChannel(-1, g_sound_chicken_hit[rand() % 2], 0);
                    // Remove the bullet
                    spaceship.RemoveBullet(sb);

                    // If boss is defeated
                    if(boss.get_heart() < 0){
                        kill++;
                        // Create explosion animation at boss position
                        int x_pos = (boss.GetRect().x + WIDTH_BOSS / 2) - WIDTH_FRAME_EXP / 2;
                        int y_pos = (boss.GetRect().y + HEIGHT_BOSS / 2) - HEIGHT_FRAME_EXP / 2;
                        exp_boss.SetRect(x_pos, y_pos);
                        exp_boss.set_frame(0);
                        // Move boss off-screen
                        boss.SetRect(SCREEN_WIDTH / 2, -SCREEN_HEIGHT);
                        // Play explosion sound
                        Mix_PlayChannel(-1, g_sound_exp[0], 0);
                    }
                }
            }
        }
    }
}

// Handle chicken enemy logic and interactions
void game_management::handle_chicken(){
    if(kill < NUMBER_OF_CHICKEN * 2){
        for(int ck = 0; ck < p_chicken_list.size(); ck++){
            Chicken* p_chicken = p_chicken_list.at(ck);
            if(p_chicken){
                // Update chicken position and rendering
                p_chicken->Move();
                p_chicken->Show(gRenderer);
                p_chicken->HandleBullet(gRenderer);
            }

            // Check collisions between chicken bullets and player
            bool Col1 = false;
            std::vector<bullet*> bullet_list = p_chicken->get_bullet_list();
            for(int b = 0; b < bullet_list.size(); b++){
                bullet* p_bullet = bullet_list.at(b);
                if(p_bullet){
                    Col1 = check_collision(p_bullet->GetRect(), spaceship.GetRect());
                    if(Col1 == true){
                        p_chicken->RemoveBullet(b);
                        break;
                    }
                }
            }

            // Check collision between chicken and player
            bool Col2 = check_collision(spaceship.GetRect(), p_chicken->GetRectFrame());

            // Handle player damage from chicken or chicken bullets
            if(Col1 || Col2){
                // Play explosion sound
                Mix_PlayChannel(-1, g_sound_exp[0], 0);

                // Create explosion animation at player position
                int x_pos = (spaceship.GetRect().x + WIDTH_MAIN / 2) - WIDTH_FRAME_EXP / 2;
                int y_pos = (spaceship.GetRect().y + HEIGHT_MAIN / 2) - HEIGHT_FRAME_EXP / 2;
                exp.SetRect(x_pos, y_pos);
                exp.set_frame(0);

                // Move player off-screen temporarily
                spaceship.SetRect(SCREEN_WIDTH*2, SCREEN_HEIGHT*2);
                spaceship.set_status(false);
                spaceship.decrease_heart();

                // Reduce weapon level when player is hit
                if(spaceship.get_heart() >= 0){
                    bullet_level = bullet_level < 2 ? 0 : (bullet_level - 1);
                }
            }

            // Check collisions between player bullets and chickens
            std::vector<bullet*> s_bullet_list = spaceship.get_bullet_list();
            for(int sb = 0; sb < s_bullet_list.size(); sb++){
                bullet* p_bullet = s_bullet_list.at(sb);
                if(p_bullet != NULL){
                    bool Col3 = check_collision(p_bullet->GetRect(), p_chicken->GetRectFrame());
                    if(Col3){
                        // Deal damage to chicken based on bullet level
                        p_chicken->Decrease((spaceship.get_bullet_damage()) + bullet_level * BULLET_DAMAGE_LEVEL_UP);
                        // Play hit sound
                        Mix_PlayChannel(-1, g_sound_chicken_hit[rand() % 2], 0);
                        // Remove the bullet
                        spaceship.RemoveBullet(sb);

                        // If chicken is defeated
                        if(p_chicken->get_heart() <= 0){
                            p_chicken->set_heart(CHICKEN_HEART);  // Reset health for next round
                            kill++;  // Increment kill counter
                            // Play chicken death sound
                            Mix_PlayChannel(-1, g_sound_chicken_hit[2], 0);

                            // Move chicken off-screen
                            p_chicken->SetRect(p_chicken->GetRect().x, -3 * SCREEN_HEIGHT);

                            // Don't bring back chickens after a certain kill count
                            if(kill > NUMBER_OF_CHICKEN){
                                p_chicken->set_come_back(false);
                            }

                            // Randomly drop power-ups
                            if(kill % ONE_TURN_GIFT == 0){
                                gift.set_gift_type(gift.random_gift());
                                gift.loadImgGift(gRenderer);
                                gift.set_y_val(GIFT_SPEED);
                                // Random horizontal position for gift
                                gift.SetRect((rand() % (SCREEN_WIDTH - 2 * GIFT_WIDTH) + GIFT_WIDTH), -GIFT_HEIGHT);
                                gift.set_come_back(true);
                            }
                        }
                    }
                }
            }
        }
    }
}

// Main game update and rendering function
void game_management::handle_game() {
    // Start timing the frame for consistent frame rate
    Uint32 frameStart = SDL_GetTicks();

    // Clear the renderer with the default color
    SDL_SetRenderDrawColor(gRenderer, RENDER_DRAW_COLOR, RENDER_DRAW_COLOR, RENDER_DRAW_COLOR, RENDER_DRAW_COLOR);
    SDL_RenderClear(gRenderer);

    // Handle scrolling background for a continuous space effect
    scrolling += SCREEN_SPEED;
    if(scrolling >= SCREEN_HEIGHT) {
        scrolling = 0; // Reset scrolling when it reaches screen height to create seamless loop
    }

    // Render two background images to create continuous scrolling effect
    background.SetRect(0, scrolling - SCREEN_HEIGHT);
    background.Render(gRenderer);
    background.SetRect(0, scrolling);
    background.Render(gRenderer);

    // Update pause button image based on current state (play/pause)
    if(pauseButtonState == 0) {
        pauseButton.loadImg("image//play.png", gRenderer);
    } else {
        pauseButton.loadImg("image//pause.png", gRenderer);
    }
    pauseButton.SetRect(pauseButtonRect.x, pauseButtonRect.y);
    pauseButton.Render(gRenderer);

    // Only process game logic if the game is not paused
    if(!isPaused) {
        // Update and render player's spaceship
        spaceship.Move();
        spaceship.Show(gRenderer);
        spaceship.HandleBullet(gRenderer);

        // Process enemies (chickens and boss)
        handle_chicken(); // Handle chicken enemies and their bullets
        handle_boss();    // Handle boss enemy and its bullets

        // Render explosion animations
        if(exp.get_frame() < NUMBER_OF_FRAME * 2) {
            exp.Show(gRenderer); // Show player's explosion if recently hit
        }

        // Handle boss explosion animation with multiple explosions for boss death
        if(exp_boss.get_frame() < NUMBER_OF_FRAME * 2) {
            exp_boss.Show(gRenderer);
            if(exp_boss.get_frame() >= NUMBER_OF_FRAME * 2 && count_num_exp < NUMBER_OF_EXP) {
                Mix_PlayChannel(-1, g_sound_exp[0], 0); // Play explosion sound
                exp_boss.set_frame(0); // Reset animation to create multiple explosions
                count_num_exp++;
            }
        }

        // Game over condition - player has no lives left
        if (spaceship.get_heart() == 0) {
            if (time_end_game < 300) { // Show game over message for 300 frames
                time_end_game++;

                end_game.SetText("Game Over !");

                // Get text dimensions for centering
                int text_width = 0, text_height = 0;
                TTF_SizeText(g_font_end_game, "Game Over !", &text_width, &text_height);

                // Center the text on screen
                end_game.SetRect((SCREEN_WIDTH - text_width) / 2, SCREEN_HEIGHT / 3);
                end_game.loadText_showText(g_font_end_game, gRenderer);
            }
            else {
                // Show menu after delay to restart game
                menu("Play Again");
                reset_game();
            }
        }
        else {
            // Player still has lives but is currently dead (waiting to respawn)
            if (spaceship.get_status() == false) {
                hint.SetText("Press 'ENTER' to revive !");

                // Center the revival hint text
                int text_width = 0, text_height = 0;
                TTF_SizeText(g_font_menu, "Press 'ENTER' to revive !", &text_width, &text_height);
                hint.SetRect((SCREEN_WIDTH - text_width) / 2, SCREEN_HEIGHT / 3);
                hint.loadText_showText(g_font_menu, gRenderer);
            }
        }

        // Victory condition - boss is defeated
        if(boss.get_heart() <= 0) {
            if(time_end_game < 300) { // Show win message for 300 frames
                time_end_game++;
                end_game.SetText("Win Game !");

                // Center the win text
                int text_width = 0, text_height = 0;
                TTF_SizeText(g_font_end_game, "Win Game !", &text_width, &text_height);
                end_game.SetRect((SCREEN_WIDTH - text_width) / 2, SCREEN_HEIGHT / 3);
                end_game.loadText_showText(g_font_end_game, gRenderer);
            }
            else {
                // Show menu after delay to restart game
                menu("Play Again");
                reset_game();
            }
        }

        // Handle power-up gifts that appear during gameplay
        gift.Move(SCREEN_WIDTH, SCREEN_HEIGHT);
        gift.Show(gRenderer);

        // Detect collision between player and gift/power-up
        bool Col4 = check_collision(spaceship.GetRect(),
                                    gift.get_gift_type() < LEVEL_UP ? gift.GetRect() : gift.GetRectFrame());
        if(Col4) {
            // Remove gift from screen after collecting
            gift.SetRect(-GIFT_WIDTH, -GIFT_HEIGHT);

            // Upgrade bullet level if appropriate gift is collected
            if(bullet_level < 3 && ((gift.get_gift_type() == spaceship.get_bul_type()) || gift.get_gift_type() == LEVEL_UP)) {
                bullet_level++;
            }

            // Play level up sound and update player's bullet type
            Mix_PlayChannel(-1, g_sound_level_up, 0);
            if(gift.get_gift_type() < LEVEL_UP) {
                spaceship.set_bul_type(gift.get_gift_type());
            }

            gift.set_come_back(false);
        }

        // Render UI elements (hearts, kill count, weapon level)
        support.Render(gRenderer);

        // Display player's remaining lives
        heart_text.SetText(std::to_string(spaceship.get_heart()));
        heart_text.SetRect(195, 15);
        heart_text.loadText_showText(g_font_text, gRenderer);

        // Display kill count
        kill_text.SetText(std::to_string(kill));
        kill_text.SetRect(50, 15);
        kill_text.loadText_showText(g_font_text, gRenderer);

        // Display current weapon/bullet level
        lighting_text.SetText(std::to_string(bullet_level));
        lighting_text.SetRect(280, 15);
        lighting_text.loadText_showText(g_font_text, gRenderer);
    }
    else {
        // If game is paused, render the pause overlay
        SDL_RenderCopy(gRenderer, pause_overlay_texture, NULL, NULL);
    }

    // Present the renderer with all drawn elements
    SDL_RenderPresent(gRenderer);

    // Maintain consistent frame rate by delaying if frame rendered too quickly
    Uint32 frameTime = SDL_GetTicks() - frameStart;
    if (frameTime < SCREEN_TICKS_PER_FRAME) {
        SDL_Delay(SCREEN_TICKS_PER_FRAME - frameTime);
    }
}
void game_management::clean(){
    SDL_DestroyWindow(gWindow);
    SDL_DestroyRenderer(gRenderer);
    gWindow = NULL;
    gRenderer = NULL;

    SDL_Quit();
    IMG_Quit();
    TTF_Quit();
    Mix_Quit();
     if (pause_overlay_texture) {
        SDL_DestroyTexture(pause_overlay_texture);
        pause_overlay_texture = nullptr;
    }
}
