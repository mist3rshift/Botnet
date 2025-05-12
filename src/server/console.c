#include <ncurses.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "../../include/server/console.h"
#include <curl/curl.h>
#include "../lib/cJSON.h"
#include <stdarg.h> // For va_list, va_start, va_end
#include <unistd.h>

// Global variable to track the number of lines used by the last print_wrapped call
int line_offset = 0;

// Helper function to write the HTTP response to a buffer
static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t total_size = size * nmemb;
    strncat((char *)userp, (char *)contents, total_size);
    return total_size;
}

// Menu options
#define NUM_OPTIONS 5
const char *menu_options[NUM_OPTIONS] = {
    "Display Bots",
    "Request Upload from Bot",
    "Send Command to Bot(s)",
    "Get Botfile's Last Lines",
    "Quit"
};

// ASCII art for "MALT"
void print_ascii_art() {
    print_wrapped(0, 0, "   __  __    _    _   _______");
    print_wrapped(1, 0, "  |  \\/  |  / \\  | | |__   __|");
    print_wrapped(2, 0, "  | |\\/| | / _ \\ | |    | |");
    print_wrapped(3, 0, "  | |  | |/ ___ \\| |___ | |");
    print_wrapped(4, 0, "  |_|  |_/_/   \\_\\_____||_|");
    print_wrapped(5, 0, "-----------------------------------");
}

// Function to calculate the number of lines a user's input will occupy
int calculate_input_lines(const char *input, int start_col, int max_x) {
    int input_len = strlen(input);
    int lines_used = 0;
    int current_col = start_col;

    for (int i = 0; i < input_len; i++) {
        if (current_col >= max_x) {
            lines_used++;
            current_col = 0;
        }
        current_col++;
    }

    if (current_col > 0) {
        lines_used++; // Account for the last line
    }

    return lines_used;
}

// Function to display the menu and handle user input
void *interactive_menu() {
    int highlight = 0; // Index of the currently highlighted option
    int choice = -1;   // User's choice
    int ch;

    // Initialize ncurses
    initscr();
    clear();
    noecho();
    cbreak();
    keypad(stdscr, TRUE); // Enable arrow keys
    start_color();
    init_pair(1, COLOR_BLUE, COLOR_BLACK); // Blue text on black background
    init_pair(2, COLOR_WHITE, COLOR_BLACK); // White text on black background
    init_pair(3, COLOR_GREEN, COLOR_BLACK); // Green for success
    init_pair(4, COLOR_RED, COLOR_BLACK);   // Red for errors

    while (1) {
        clear();
        print_ascii_art();

        // Display the menu
        int current_row = 7; // Start at row 7
        for (int i = 0; i < NUM_OPTIONS; i++) {
            if (i == highlight) {
                attron(A_BOLD | COLOR_PAIR(1)); // Highlight the selected option in blue
                mvprintw(current_row, 0, ">"); // Add the ">" symbol
                mvprintw(current_row, 2, "%s", menu_options[i]);
                attroff(A_BOLD | COLOR_PAIR(1));
            } else {
                attron(COLOR_PAIR(2)); // Normal options in white
                mvprintw(current_row, 0, " "); // Clear the ">" symbol for non-selected options
                mvprintw(current_row, 2, "%s", menu_options[i]);
                attroff(COLOR_PAIR(2));
            }
            current_row++; // Move to the next row
        }

        refresh();

        // Handle user input
        ch = getch();
        if (ch == 'q') {
            break; // Exit on 'q'
        } else if (ch == KEY_UP) {
            highlight = (highlight - 1 + NUM_OPTIONS) % NUM_OPTIONS; // Move up
        } else if (ch == KEY_DOWN) {
            highlight = (highlight + 1) % NUM_OPTIONS; // Move down
        } else if (ch == '\n') { // Enter key
            choice = highlight;
        }

        // If the user presses Enter, handle the selected option
        if (choice != -1) {
            clear();
            if (choice == NUM_OPTIONS - 1) { // Quit option
                attron(A_BOLD | COLOR_PAIR(4));
                print_wrapped(0, 0, "Exiting CLI. Goodbye!");
                attroff(A_BOLD | COLOR_PAIR(4));
                refresh();
                break;
            } else {
                print_wrapped(0, 0, "%s", menu_options[choice]);
                refresh();
                // Call the corresponding function based on the choice
                switch (choice) {
                    case 0:
                        display_bots();
                        break;
                    case 1:
                        get_file_from_bot();
                        break;
                    case 2:
                        send_command_to_bot();
                        break;
                    case 3:
                        get_bot_file();
                        break;
                }
            }
            choice = -1; // Reset choice
        }
    }

    // End ncurses mode
    endwin();
}

void display_bots() {
    CURL *curl = curl_easy_init();
    if (!curl) {
        attron(A_BOLD | COLOR_PAIR(4));
        print_wrapped(4, 0, "Failed to initialize CURL.");
        attroff(A_BOLD | COLOR_PAIR(4));
        refresh();
        return;
    }

    char response[4096] = {0}; // Buffer to store the HTTP response

    // Set up the CURL request
    curl_easy_setopt(curl, CURLOPT_URL, "http://127.0.0.1:8000/api/bots");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        attron(A_BOLD | COLOR_PAIR(4));
        print_wrapped(4, 0, "Failed to fetch bots: %s", curl_easy_strerror(res));
        attroff(A_BOLD | COLOR_PAIR(4));
        refresh();
        curl_easy_cleanup(curl);
        return;
    }

    curl_easy_cleanup(curl);

    // Parse the JSON response using cJSON
    cJSON *parsed_json = cJSON_Parse(response);
    if (!parsed_json) {
        attron(A_BOLD | COLOR_PAIR(4));
        print_wrapped(4, 0, "Failed to parse JSON response.");
        attroff(A_BOLD | COLOR_PAIR(4));
        refresh();
        return;
    }

    // Ensure the response is an array
    if (!cJSON_IsArray(parsed_json)) {
        attron(A_BOLD | COLOR_PAIR(4));
        print_wrapped(4, 0, "Unexpected response format. Expected an array.");
        attroff(A_BOLD | COLOR_PAIR(4));
        refresh();
        cJSON_Delete(parsed_json); // Free the JSON object
        return;
    }

    // Prepare for scrolling
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x); // Get screen dimensions
    int start_line = 0;             // Line to start displaying from
    int total_lines = cJSON_GetArraySize(parsed_json); // Total lines in the response

    while (1) {
        clear();
        attron(A_BOLD | COLOR_PAIR(3));
        mvprintw(0, 0, "Connected Bots (Press 'q' to exit):");
        attroff(A_BOLD | COLOR_PAIR(3));

        int display_lines = max_y - 3; // Lines available for display (excluding header and footer)
        if (total_lines <= 0) {
            attron(A_BOLD | COLOR_PAIR(4));
            mvprintw(start_line+1, 0, "No bots to display");
            attroff(A_BOLD | COLOR_PAIR(4));
        }
        for (int i = 0; i < display_lines && (start_line + i) < total_lines; i++) {
            cJSON *bot = cJSON_GetArrayItem(parsed_json, start_line + i);
            if (cJSON_IsObject(bot)) {
                cJSON *socket_obj = cJSON_GetObjectItem(bot, "socket");
                cJSON *id_obj = cJSON_GetObjectItem(bot, "id");
                cJSON *state_obj = cJSON_GetObjectItem(bot, "status");

                if (cJSON_IsString(socket_obj) && cJSON_IsString(id_obj) && cJSON_IsString(state_obj)) {
                    mvprintw(i + 1, 0, "%-20s %-20s %-20s",
                             socket_obj->valuestring,
                             id_obj->valuestring,
                             state_obj->valuestring);
                }
            }
        }

        // Add a footer at the bottom
        attron(A_BOLD | COLOR_PAIR(2));
        mvprintw(max_y - 1, 0, "Press 'q' to exit | Use Arrow Keys to Scroll");
        attroff(A_BOLD | COLOR_PAIR(2));

        refresh();

        // Use the reusable input loop for scrolling and quitting
        if (handle_scrolling_and_quitting(&start_line, total_lines, display_lines)) {
            break; // Exit if 'q' is pressed
        }
    }

    // Free the JSON object
    cJSON_Delete(parsed_json);
}

void get_file_from_bot() {
    CURL *curl = curl_easy_init();
    if (!curl) {
        clear();
        attron(A_BOLD | COLOR_PAIR(4));
        print_wrapped(4, 0, "Failed to initialize CURL.");
        attroff(A_BOLD | COLOR_PAIR(4));
        refresh();
        getch(); // Wait for user input before returning
        return;
    }

    char response[4096] = {0}; // Buffer to store the HTTP response

    // Fetch connected bots
    curl_easy_setopt(curl, CURLOPT_URL, "http://127.0.0.1:8000/api/bots");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        clear();
        attron(A_BOLD | COLOR_PAIR(4));
        print_wrapped(4, 0, "Failed to fetch bots: %s", curl_easy_strerror(res));
        attroff(A_BOLD | COLOR_PAIR(4));
        refresh();
        getch(); // Wait for user input before returning
        curl_easy_cleanup(curl);
        return;
    }

    curl_easy_cleanup(curl);

    // Parse the JSON response using cJSON
    cJSON *parsed_json = cJSON_Parse(response);
    if (!parsed_json) {
        clear();
        attron(A_BOLD | COLOR_PAIR(4));
        print_wrapped(4, 0, "Failed to parse JSON response.");
        attroff(A_BOLD | COLOR_PAIR(4));
        refresh();
        getch(); // Wait for user input before returning
        return;
    }

    // Ensure the response is an array
    if (!cJSON_IsArray(parsed_json)) {
        clear();
        attron(A_BOLD | COLOR_PAIR(4));
        print_wrapped(4, 0, "Unexpected response format. Expected an array.");
        attroff(A_BOLD | COLOR_PAIR(4));
        refresh();
        getch(); // Wait for user input before returning
        cJSON_Delete(parsed_json); // Free the JSON object
        return;
    }

    // Check if there are any bots
    int total_bots = cJSON_GetArraySize(parsed_json);
    if (total_bots == 0) {
        clear();
        attron(A_BOLD | COLOR_PAIR(4));
        print_wrapped(4, 0, "No bots available.");
        attroff(A_BOLD | COLOR_PAIR(4));
        refresh();
        getch(); // Wait for user input before returning
        cJSON_Delete(parsed_json);
        return;
    }

    // Create a menu for selecting a bot
    int highlight = 0; // Index of the currently highlighted bot
    int ch;
    char selected_bot[256] = "None"; // Store the selected bot for the footer

    while (1) {
        clear();
        attron(A_BOLD | COLOR_PAIR(3));
        mvprintw(0, 0, "Select a Bot (Press 'q' to cancel):");
        attroff(A_BOLD | COLOR_PAIR(3));

        // Display the list of bots
        for (int i = 0; i < total_bots; i++) {
            cJSON *bot = cJSON_GetArrayItem(parsed_json, i);
            cJSON *id_obj = cJSON_GetObjectItem(bot, "id");

            if (cJSON_IsString(id_obj)) {
                if (i == highlight) {
                    attron(A_BOLD | COLOR_PAIR(1)); // Highlight the selected bot in blue
                    mvprintw(i + 1, 0, "> %s", id_obj->valuestring);
                    attroff(A_BOLD | COLOR_PAIR(1));
                    // Update the footer with the currently highlighted bot
                    strncpy(selected_bot, id_obj->valuestring, sizeof(selected_bot) - 1);
                    selected_bot[sizeof(selected_bot) - 1] = '\0'; // Ensure null termination
                } else {
                    attron(COLOR_PAIR(2)); // Normal bots in white
                    mvprintw(i + 1, 0, "  %s", id_obj->valuestring);
                    attroff(COLOR_PAIR(2));
                }
            }
        }

        // Add a footer at the bottom
        attron(A_BOLD | COLOR_PAIR(2));
        mvprintw(LINES - 1, 0, "Selected Bot: %s | Press 'q' to cancel", selected_bot);
        attroff(A_BOLD | COLOR_PAIR(2));

        refresh();

        // Handle user input
        ch = getch();
        if (ch == 'q') {
            cJSON_Delete(parsed_json);
            return; // Exit on 'q'
        } else if (ch == KEY_UP) {
            highlight = (highlight - 1 + total_bots) % total_bots; // Move up
        } else if (ch == KEY_DOWN) {
            highlight = (highlight + 1) % total_bots; // Move down
        } else if (ch == '\n') { // Enter key
            // Get the selected bot's ID
            cJSON *selected_bot_obj = cJSON_GetArrayItem(parsed_json, highlight);
            cJSON *id_obj = cJSON_GetObjectItem(selected_bot_obj, "id");
            if (cJSON_IsString(id_obj)) {
                strncpy(selected_bot, id_obj->valuestring, sizeof(selected_bot) - 1);
                selected_bot[sizeof(selected_bot) - 1] = '\0'; // Ensure null termination
            }
            break; // Exit the menu
        }
    }

    cJSON_Delete(parsed_json); // Free the JSON object

    // Prompt the user for the number of lines to fetch
    char lines_str[16] = {0};
    print_wrapped(2, 0, "Enter the number of lines to fetch: ");
    echo();
    getnstr(lines_str, sizeof(lines_str) - 1);
    noecho();

    // Validate the number of lines
    if (strlen(lines_str) == 0 || atoi(lines_str) <= 0) {
        attron(A_BOLD | COLOR_PAIR(4));
        print_wrapped(4, 0, "Invalid number of lines. Please enter a positive integer.");
        attroff(A_BOLD | COLOR_PAIR(4));
        refresh();
        return;
    }

    // Prompt the user for the offset
    char offset_str[16] = {0};
    print_wrapped(3, 0, "Enter the offset (number of most recent lines to ignore): ");
    echo();
    getnstr(offset_str, sizeof(offset_str) - 1);
    noecho();

    // Validate the offset
    if (strlen(offset_str) == 0 || atoi(offset_str) < 0) {
        attron(A_BOLD | COLOR_PAIR(4));
        print_wrapped(4, 0, "Invalid offset. Please enter a non-negative integer.");
        attroff(A_BOLD | COLOR_PAIR(4));
        refresh();
        return;
    }

    // Construct the query URL
    char query_url[512];
    snprintf(query_url, sizeof(query_url),
             "http://127.0.0.1:8000/api/botfile?id=%s&lines=%s&offset=%s",
             selected_bot, lines_str, offset_str);

    // Fetch the bot file content
    curl = curl_easy_init();
    if (!curl) {
        attron(A_BOLD | COLOR_PAIR(4));
        print_wrapped(4, 0, "Failed to initialize CURL.");
        attroff(A_BOLD | COLOR_PAIR(4));
        refresh();
        return;
    }

    memset(response, 0, sizeof(response)); // Clear the response buffer
    curl_easy_setopt(curl, CURLOPT_URL, query_url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        attron(A_BOLD | COLOR_PAIR(4));
        print_wrapped(4, 0, "Failed to fetch bot file: %s", curl_easy_strerror(res));
        attroff(A_BOLD | COLOR_PAIR(4));
        refresh();
        curl_easy_cleanup(curl);
        return;
    }

    curl_easy_cleanup(curl);

    // Parse the JSON response
    cJSON *parsed_file = cJSON_Parse(response);
    if (!parsed_file) {
        attron(A_BOLD | COLOR_PAIR(4));
        print_wrapped(4, 0, "Failed to parse JSON response.");
        attroff(A_BOLD | COLOR_PAIR(4));
        refresh();
        return;
    }

    // Extract the "lines" array from the response
    cJSON *lines_obj = cJSON_GetObjectItem(parsed_file, "lines");
    if (!cJSON_IsArray(lines_obj)) {
        attron(A_BOLD | COLOR_PAIR(4));
        print_wrapped(4, 0, "Unexpected response format. 'lines' array not found.");
        attroff(A_BOLD | COLOR_PAIR(4));
        cJSON_Delete(parsed_file);
        refresh();
        return;
    }

    // Prepare for scrolling
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x); // Get screen dimensions
    int start_line = 0;             // Line to start displaying from
    int total_lines = cJSON_GetArraySize(lines_obj); // Total lines in the response

    while (1) {
        clear();
        attron(A_BOLD | COLOR_PAIR(3));
        mvprintw(0, 0, "Bot file content (Press 'q' to exit):");
        attroff(A_BOLD | COLOR_PAIR(3));

        int display_lines = max_y - 3; // Lines available for display (excluding header and footer)
        for (int i = 0; i < display_lines && (start_line + i) < total_lines; i++) {
            cJSON *line = cJSON_GetArrayItem(lines_obj, start_line + i);
            if (cJSON_IsString(line)) {
                mvprintw(i + 1, 0, "%s", line->valuestring);
            }
        }

        // Add a footer at the bottom
        attron(A_BOLD | COLOR_PAIR(2));
        mvprintw(max_y - 1, 0, "Press 'q' to exit | Use Arrow Keys to Scroll");
        attroff(A_BOLD | COLOR_PAIR(2));

        refresh();

        // Use the reusable input loop for scrolling and quitting
        if (handle_scrolling_and_quitting(&start_line, total_lines, display_lines)) {
            break; // Exit if 'q' is pressed
        }
    }

    // Free the JSON object
    cJSON_Delete(parsed_file);
}

void send_command_to_bot() {
    CURL *curl = curl_easy_init();
    if (!curl) {
        clear();
        attron(A_BOLD | COLOR_PAIR(4));
        print_wrapped(4, 0, "Failed to initialize CURL.");
        attroff(A_BOLD | COLOR_PAIR(4));
        refresh();
        getch(); // Wait for user input before returning
        return;
    }

    char response[4096] = {0}; // Buffer to store the HTTP response

    // Fetch connected bots
    curl_easy_setopt(curl, CURLOPT_URL, "http://127.0.0.1:8000/api/bots");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        clear();
        attron(A_BOLD | COLOR_PAIR(4));
        print_wrapped(4, 0, "Failed to fetch bots: %s", curl_easy_strerror(res));
        attroff(A_BOLD | COLOR_PAIR(4));
        refresh();
        getch(); // Wait for user input before returning
        curl_easy_cleanup(curl);
        return;
    }

    curl_easy_cleanup(curl);

    // Parse the JSON response using cJSON
    cJSON *parsed_json = cJSON_Parse(response);
    if (!parsed_json) {
        clear();
        attron(A_BOLD | COLOR_PAIR(4));
        print_wrapped(4, 0, "Failed to parse JSON response.");
        attroff(A_BOLD | COLOR_PAIR(4));
        refresh();
        getch(); // Wait for user input before returning
        return;
    }

    // Ensure the response is an array
    if (!cJSON_IsArray(parsed_json)) {
        clear();
        attron(A_BOLD | COLOR_PAIR(4));
        print_wrapped(4, 0, "Unexpected response format. Expected an array.");
        attroff(A_BOLD | COLOR_PAIR(4));
        refresh();
        getch(); // Wait for user input before returning
        cJSON_Delete(parsed_json); // Free the JSON object
        return;
    }

    // Prepare for bot selection
    int total_bots = cJSON_GetArraySize(parsed_json);

    // List of selected bots
    char selected_bots[1024] = {0}; // Comma-separated list of selected bot IDs
    int highlight = 0;              // Index of the currently highlighted bot
    int ch;

    while (1) {
        clear();
        attron(A_BOLD | COLOR_PAIR(3));
        mvprintw(0, 0, "Send Bot Commands (Press 'A' to Add, 'R' to Remove, 'C' to Continue):");
        attroff(A_BOLD | COLOR_PAIR(3));

        // Display the list of selected bots
        attron(COLOR_PAIR(2));
        mvprintw(2, 0, "Selected Bots: %s", strlen(selected_bots) > 0 ? selected_bots : "None");
        attroff(COLOR_PAIR(2));

        // Add a footer to display options
        attron(A_BOLD | COLOR_PAIR(2));
        mvprintw(LINES - 1, 0, "Options: [A] Add Bot | [R] Remove Bot | [C] Continue");
        attroff(A_BOLD | COLOR_PAIR(2));

        refresh();

        // Handle user input
        ch = getch();
        if (ch == 'q') {
            cJSON_Delete(parsed_json);
            return; // Exit on 'q'
        } else if (ch == 'A' || ch == 'a') {
            // Add a bot
            if (total_bots == 0) {
                clear();
                attron(A_BOLD | COLOR_PAIR(4));
                print_wrapped(4, 0, "No bots to select.");
                attroff(A_BOLD | COLOR_PAIR(4));
                refresh();
                getch(); // Wait for user input before returning
                continue;
            }

            // Parse the selected_bots string into an array
            char *selected_bot_list[256];
            int selected_count = 0;
            char temp_selected_bots[1024];
            strncpy(temp_selected_bots, selected_bots, sizeof(temp_selected_bots) - 1);
            temp_selected_bots[sizeof(temp_selected_bots) - 1] = '\0';

            char *token = strtok(temp_selected_bots, ",");
            while (token != NULL && selected_count < 256) {
                selected_bot_list[selected_count++] = token;
                token = strtok(NULL, ",");
            }

            // Create a filtered list of bots
            int filtered_count = 0;
            cJSON *filtered_bots[256];
            for (int i = 0; i < total_bots; i++) {
                cJSON *bot = cJSON_GetArrayItem(parsed_json, i);
                cJSON *id_obj = cJSON_GetObjectItem(bot, "id");

                if (cJSON_IsString(id_obj)) {
                    bool already_selected = false;
                    for (int j = 0; j < selected_count; j++) {
                        if (strcmp(id_obj->valuestring, selected_bot_list[j]) == 0) {
                            already_selected = true;
                            break;
                        }
                    }
                    if (!already_selected) {
                        filtered_bots[filtered_count++] = bot;
                    }
                }
            }

            // Display the filtered list of bots
            highlight = 0; // Reset highlight for the add menu
            while (1) {
                clear();
                attron(A_BOLD | COLOR_PAIR(3));
                mvprintw(0, 0, "Select a Bot to Add (Press 'q' to Cancel):");
                attroff(A_BOLD | COLOR_PAIR(3));

                for (int i = 0; i < filtered_count; i++) {
                    cJSON *bot = filtered_bots[i];
                    cJSON *id_obj = cJSON_GetObjectItem(bot, "id");

                    if (cJSON_IsString(id_obj)) {
                        if (i == highlight) {
                            attron(A_BOLD | COLOR_PAIR(1)); // Highlight the selected bot in blue
                            mvprintw(i + 1, 0, "> %s", id_obj->valuestring);
                            attroff(A_BOLD | COLOR_PAIR(1));
                        } else {
                            attron(COLOR_PAIR(2)); // Normal bots in white
                            mvprintw(i + 1, 0, "  %s", id_obj->valuestring);
                            attroff(COLOR_PAIR(2));
                        }
                    }
                }

                refresh();

                // Handle user input
                ch = getch();
                if (ch == 'q') {
                    break; // Cancel adding a bot
                } else if (ch == KEY_UP) {
                    highlight = (highlight - 1 + filtered_count) % filtered_count; // Move up
                } else if (ch == KEY_DOWN) {
                    highlight = (highlight + 1) % filtered_count; // Move down
                } else if (ch == '\n') { // Enter key
                    // Add the selected bot to the list
                    cJSON *selected_bot = filtered_bots[highlight];
                    cJSON *id_obj = cJSON_GetObjectItem(selected_bot, "id");
                    if (cJSON_IsString(id_obj)) {
                        if (strlen(selected_bots) > 0) {
                            strncat(selected_bots, ",", sizeof(selected_bots) - strlen(selected_bots) - 1);
                        }
                        strncat(selected_bots, id_obj->valuestring, sizeof(selected_bots) - strlen(selected_bots) - 1);
                    }
                    break; // Return to the previous menu
                }
            }
        } else if (ch == 'R' || ch == 'r') {
            // Remove a bot
            if (strlen(selected_bots) == 0) {
                clear();
                attron(A_BOLD | COLOR_PAIR(4));
                print_wrapped(4, 0, "No bots to remove.");
                attroff(A_BOLD | COLOR_PAIR(4));
                refresh();
                getch(); // Wait for user input before returning
                continue;
            }

            // Split the selected bots into an array
            char *bot_list[256];
            int bot_count = 0;
            char temp_selected_bots[1024];
            strncpy(temp_selected_bots, selected_bots, sizeof(temp_selected_bots) - 1);
            temp_selected_bots[sizeof(temp_selected_bots) - 1] = '\0';

            char *token = strtok(temp_selected_bots, ",");
            while (token != NULL && bot_count < 256) {
                bot_list[bot_count++] = token;
                token = strtok(NULL, ",");
            }

            highlight = 0; // Reset highlight for the remove menu
            while (1) {
                clear();
                attron(A_BOLD | COLOR_PAIR(3));
                mvprintw(0, 0, "Select a Bot to Remove (Press 'q' to Cancel):");
                attroff(A_BOLD | COLOR_PAIR(3));

                // Display the list of selected bots
                for (int i = 0; i < bot_count; i++) {
                    if (i == highlight) {
                        attron(A_BOLD | COLOR_PAIR(4)); // Highlight the selected bot in red
                        mvprintw(i + 1, 0, "> %s", bot_list[i]);
                        attroff(A_BOLD | COLOR_PAIR(4));
                    } else {
                        attron(COLOR_PAIR(2)); // Normal bots in white
                        mvprintw(i + 1, 0, "  %s", bot_list[i]);
                        attroff(COLOR_PAIR(2));
                    }
                }

                refresh();

                // Handle user input
                ch = getch();
                if (ch == 'q') {
                    break; // Cancel removing a bot
                } else if (ch == KEY_UP) {
                    highlight = (highlight - 1 + bot_count) % bot_count; // Move up
                } else if (ch == KEY_DOWN) {
                    highlight = (highlight + 1) % bot_count; // Move down
                } else if (ch == '\n') { // Enter key
                    // Remove the selected bot from the list
                    for (int i = highlight; i < bot_count - 1; i++) {
                        bot_list[i] = bot_list[i + 1];
                    }
                    bot_count--;

                    // Rebuild the selected_bots string
                    selected_bots[0] = '\0';
                    for (int i = 0; i < bot_count; i++) {
                        if (i > 0) {
                            strncat(selected_bots, ",", sizeof(selected_bots) - strlen(selected_bots) - 1);
                        }
                        strncat(selected_bots, bot_list[i], sizeof(selected_bots) - strlen(selected_bots) - 1);
                    }
                    break; // Return to the previous menu
                }
            }
        } else if (ch == 'C' || ch == 'c') {
            // Continue to the next step
            break; // Exit the menu loop and proceed
        }
    }

    cJSON_Delete(parsed_json); // Free the JSON object

    // Proceed to the next step (e.g., command input)
    char num_clients_str[16] = {0};
    char cmd_id[64] = {0};
    char program[256] = {0};
    char params[256] = {0};
    char delay_str[16] = {0};
    char expected_code_str[16] = {0};
    char post_data[2048];

    // Update the footer to reflect the selected bots or number of clients
    clear();
    if (strlen(selected_bots) > 0) {
        attron(A_BOLD | COLOR_PAIR(2));
        mvprintw(LINES - 1, 0, "Selected Bots: %s", selected_bots);
        attroff(A_BOLD | COLOR_PAIR(2));
    } else {
        print_wrapped(3, 0, "Enter the number of clients to target (leave empty if using bot IDs): ");
        echo();
        getnstr(num_clients_str, sizeof(num_clients_str) - 1);
        noecho();

        attron(A_BOLD | COLOR_PAIR(2));
        mvprintw(LINES - 1, 0, "Number of Clients: %s", strlen(num_clients_str) > 0 ? num_clients_str : "None");
        attroff(A_BOLD | COLOR_PAIR(2));
    }

    // Prompt the user for the command ID
    print_wrapped(4, 0, "Enter Command ID: ");
    echo();
    getnstr(cmd_id, sizeof(cmd_id) - 1);
    noecho();

    // Prompt the user for the program to execute
    print_wrapped(5, 0, "Enter Program to Execute: ");
    echo();
    getnstr(program, sizeof(program) - 1);
    noecho();

    // Prompt the user for parameters
    print_wrapped(6, 0, "Enter Parameters (space-separated, or leave empty): ");
    echo();
    getnstr(params, sizeof(params) - 1);
    noecho();

    // Prompt the user for the delay
    print_wrapped(7, 0, "Enter Delay (in seconds, or leave empty for 0): ");
    echo();
    getnstr(delay_str, sizeof(delay_str) - 1);
    noecho();

    // Prompt the user for the expected exit code
    print_wrapped(8, 0, "Enter Expected Exit Code (or leave empty for 0): ");
    echo();
    getnstr(expected_code_str, sizeof(expected_code_str) - 1);
    noecho();

    // Construct the POST data
    snprintf(post_data, sizeof(post_data),
             "bot_ids=%s&num_clients=%s&cmd_id=%s&program=%s&params=%s&delay=%s&expected_code=%s",
             selected_bots, num_clients_str, cmd_id, program, params, delay_str, expected_code_str);

    curl = curl_easy_init();
    if (!curl) {
        attron(A_BOLD | COLOR_PAIR(4));
        print_wrapped(9, 0, "Failed to initialize CURL.");
        attroff(A_BOLD | COLOR_PAIR(4));
        refresh();
        return;
    }

    // Set up the CURL request
    curl_easy_setopt(curl, CURLOPT_URL, "http://127.0.0.1:8000/api/command");
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        attron(A_BOLD | COLOR_PAIR(4));
        print_wrapped(10, 0, "Failed to send command: %s", curl_easy_strerror(res));
        attroff(A_BOLD | COLOR_PAIR(4));
        refresh();
        curl_easy_cleanup(curl);
        return;
    }

    curl_easy_cleanup(curl);

    // Display the server's response
    attron(A_BOLD | COLOR_PAIR(3));
    print_wrapped(11, 0, "Command sent successfully. Server response:");
    attroff(A_BOLD | COLOR_PAIR(3));
    print_wrapped(12, 0, "%s", response);
    refresh();
    getch(); // Wait for user input before returning
}

void print_wrapped(int start_row, int start_col, const char *format, ...) {
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x); // Get the screen dimensions

    int row = start_row;
    int col = start_col;

    // Format the input string
    char formatted_text[4096]; // Buffer for the formatted string
    va_list args;
    va_start(args, format);
    vsnprintf(formatted_text, sizeof(formatted_text), format, args);
    va_end(args);

    int text_len = strlen(formatted_text);

    // Calculate how many lines the text will occupy
    int lines_used = 0;
    for (int i = 0, current_col = col; i < text_len; i++) {
        if (formatted_text[i] == '\n' || current_col >= max_x) {
            lines_used++;
            current_col = 0;
        }
        if (formatted_text[i] != '\n') {
            current_col++;
        }
    }
    lines_used++; // Account for the last line

    // Print the formatted string with wrapping
    for (int i = 0; i < text_len; i++) {
        if (col >= max_x || formatted_text[i] == '\n') { // Handle wrapping or explicit newlines
            col = 0;
            row++;
            if (row >= max_y) { // Prevent writing outside the screen
                break;
            }
            if (formatted_text[i] == '\n') {
                continue;
            }
        }
        mvaddch(row, col++, formatted_text[i]); // Print one character at a time
    }

    // Update the global line offset
    line_offset = lines_used;
}

void get_user_input(int start_row, int start_col, char *buffer, size_t buffer_size) {
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x); // Get the screen dimensions

    echo();
    mvgetnstr(start_row, start_col, buffer, buffer_size - 1);
    noecho();

    // Calculate the number of lines the user's input occupies
    int input_lines = calculate_input_lines(buffer, start_row, max_x);
    line_offset += input_lines; // Add the input lines to the global line offset
}

bool handle_scrolling_and_quitting(int *start_line, int total_lines, int display_lines) {
    int ch;
    while (1) {
        ch = getch();
        if (ch == 'q') {
            return true; // Exit on 'q'
        } else if (ch == KEY_DOWN) {
            if (*start_line + display_lines < total_lines) {
                (*start_line)++; // Scroll down
            }
        } else if (ch == KEY_UP) {
            if (*start_line > 0) {
                (*start_line)--; // Scroll up
            }
        } else if (ch == KEY_MOUSE) {
            // Ignore mouse events to allow text selection
            continue;
        }
        return false; // Continue scrolling
    }
}

void get_bot_file() {
    CURL *curl = curl_easy_init();
    if (!curl) {
        clear();
        attron(A_BOLD | COLOR_PAIR(4));
        print_wrapped(4, 0, "Failed to initialize CURL.");
        attroff(A_BOLD | COLOR_PAIR(4));
        refresh();
        getch(); // Wait for user input before returning
        return;
    }

    char response[4096] = {0}; // Buffer to store the HTTP response

    // Fetch connected bots
    curl_easy_setopt(curl, CURLOPT_URL, "http://127.0.0.1:8000/api/bots");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        clear();
        attron(A_BOLD | COLOR_PAIR(4));
        print_wrapped(4, 0, "Failed to fetch bots: %s", curl_easy_strerror(res));
        attroff(A_BOLD | COLOR_PAIR(4));
        refresh();
        getch(); // Wait for user input before returning
        curl_easy_cleanup(curl);
        return;
    }

    curl_easy_cleanup(curl);

    // Parse the JSON response using cJSON
    cJSON *parsed_json = cJSON_Parse(response);
    if (!parsed_json) {
        clear();
        attron(A_BOLD | COLOR_PAIR(4));
        print_wrapped(4, 0, "Failed to parse JSON response.");
        attroff(A_BOLD | COLOR_PAIR(4));
        refresh();
        getch(); // Wait for user input before returning
        return;
    }

    // Ensure the response is an array
    if (!cJSON_IsArray(parsed_json)) {
        clear();
        attron(A_BOLD | COLOR_PAIR(4));
        print_wrapped(4, 0, "Unexpected response format. Expected an array.");
        attroff(A_BOLD | COLOR_PAIR(4));
        refresh();
        getch(); // Wait for user input before returning
        cJSON_Delete(parsed_json); // Free the JSON object
        return;
    }

    // Check if there are any bots
    int total_bots = cJSON_GetArraySize(parsed_json);
    if (total_bots == 0) {
        clear();
        attron(A_BOLD | COLOR_PAIR(4));
        print_wrapped(4, 0, "No bots available.");
        attroff(A_BOLD | COLOR_PAIR(4));
        refresh();
        getch(); // Wait for user input before returning
        cJSON_Delete(parsed_json);
        return;
    }

    // Create a menu for selecting a bot
    int highlight = 0; // Index of the currently highlighted bot
    int ch;
    char selected_bot[256] = "None"; // Store the selected bot for the footer

    while (1) {
        clear();
        attron(A_BOLD | COLOR_PAIR(3));
        mvprintw(0, 0, "Select a Bot (Press 'q' to cancel):");
        attroff(A_BOLD | COLOR_PAIR(3));

        // Display the list of bots
        for (int i = 0; i < total_bots; i++) {
            cJSON *bot = cJSON_GetArrayItem(parsed_json, i);
            cJSON *id_obj = cJSON_GetObjectItem(bot, "id");

            if (cJSON_IsString(id_obj)) {
                if (i == highlight) {
                    attron(A_BOLD | COLOR_PAIR(1)); // Highlight the selected bot in blue
                    mvprintw(i + 1, 0, "> %s", id_obj->valuestring);
                    attroff(A_BOLD | COLOR_PAIR(1));
                    // Update the footer with the currently highlighted bot
                    strncpy(selected_bot, id_obj->valuestring, sizeof(selected_bot) - 1);
                    selected_bot[sizeof(selected_bot) - 1] = '\0'; // Ensure null termination
                } else {
                    attron(COLOR_PAIR(2)); // Normal bots in white
                    mvprintw(i + 1, 0, "  %s", id_obj->valuestring);
                    attroff(COLOR_PAIR(2));
                }
            }
        }

        // Add a footer at the bottom
        attron(A_BOLD | COLOR_PAIR(2));
        mvprintw(LINES - 1, 0, "Selected Bot: %s | Press 'q' to cancel", selected_bot);
        attroff(A_BOLD | COLOR_PAIR(2));

        refresh();

        // Handle user input
        ch = getch();
        if (ch == 'q') {
            cJSON_Delete(parsed_json);
            return; // Exit on 'q'
        } else if (ch == KEY_UP) {
            highlight = (highlight - 1 + total_bots) % total_bots; // Move up
        } else if (ch == KEY_DOWN) {
            highlight = (highlight + 1) % total_bots; // Move down
        } else if (ch == '\n') { // Enter key
            // Get the selected bot's ID
            cJSON *selected_bot_obj = cJSON_GetArrayItem(parsed_json, highlight);
            cJSON *id_obj = cJSON_GetObjectItem(selected_bot_obj, "id");
            if (cJSON_IsString(id_obj)) {
                strncpy(selected_bot, id_obj->valuestring, sizeof(selected_bot) - 1);
                selected_bot[sizeof(selected_bot) - 1] = '\0'; // Ensure null termination
            }
            break; // Exit the menu
        }
    }

    cJSON_Delete(parsed_json); // Free the JSON object

    // Prompt the user for the number of lines to fetch
    char lines_str[16] = {0};
    print_wrapped(2, 0, "Enter the number of lines to fetch: ");
    echo();
    getnstr(lines_str, sizeof(lines_str) - 1);
    noecho();

    // Validate the number of lines
    if (strlen(lines_str) == 0 || atoi(lines_str) <= 0) {
        attron(A_BOLD | COLOR_PAIR(4));
        print_wrapped(4, 0, "Invalid number of lines. Please enter a positive integer.");
        attroff(A_BOLD | COLOR_PAIR(4));
        refresh();
        return;
    }

    // Prompt the user for the offset
    char offset_str[16] = {0};
    print_wrapped(3, 0, "Enter the offset (number of most recent lines to ignore): ");
    echo();
    getnstr(offset_str, sizeof(offset_str) - 1);
    noecho();

    // Validate the offset
    if (strlen(offset_str) == 0 || atoi(offset_str) < 0) {
        attron(A_BOLD | COLOR_PAIR(4));
        print_wrapped(4, 0, "Invalid offset. Please enter a non-negative integer.");
        attroff(A_BOLD | COLOR_PAIR(4));
        refresh();
        return;
    }

    // Construct the query URL
    char query_url[512];
    snprintf(query_url, sizeof(query_url),
             "http://127.0.0.1:8000/api/botfile?id=%s&lines=%s&offset=%s",
             selected_bot, lines_str, offset_str);

    // Fetch the bot file content
    curl = curl_easy_init();
    if (!curl) {
        attron(A_BOLD | COLOR_PAIR(4));
        print_wrapped(4, 0, "Failed to initialize CURL.");
        attroff(A_BOLD | COLOR_PAIR(4));
        refresh();
        return;
    }

    memset(response, 0, sizeof(response)); // Clear the response buffer
    curl_easy_setopt(curl, CURLOPT_URL, query_url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        attron(A_BOLD | COLOR_PAIR(4));
        print_wrapped(4, 0, "Failed to fetch bot file: %s", curl_easy_strerror(res));
        attroff(A_BOLD | COLOR_PAIR(4));
        refresh();
        curl_easy_cleanup(curl);
        return;
    }

    curl_easy_cleanup(curl);

    // Parse the JSON response
    cJSON *parsed_file = cJSON_Parse(response);
    if (!parsed_file) {
        attron(A_BOLD | COLOR_PAIR(4));
        print_wrapped(4, 0, "Failed to parse JSON response.");
        attroff(A_BOLD | COLOR_PAIR(4));
        refresh();
        return;
    }

    // Extract the "lines" array from the response
    cJSON *lines_obj = cJSON_GetObjectItem(parsed_file, "lines");
    if (!cJSON_IsArray(lines_obj)) {
        attron(A_BOLD | COLOR_PAIR(4));
        print_wrapped(4, 0, "Unexpected response format. 'lines' array not found.");
        attroff(A_BOLD | COLOR_PAIR(4));
        cJSON_Delete(parsed_file);
        refresh();
        return;
    }

    // Prepare for scrolling
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x); // Get screen dimensions
    int start_line = 0;             // Line to start displaying from
    int total_lines = cJSON_GetArraySize(lines_obj); // Total lines in the response

    while (1) {
        clear();
        attron(A_BOLD | COLOR_PAIR(3));
        mvprintw(0, 0, "Bot file content (Press 'q' to exit):");
        attroff(A_BOLD | COLOR_PAIR(3));

        int display_lines = max_y - 3; // Lines available for display (excluding header and footer)
        for (int i = 0; i < display_lines && (start_line + i) < total_lines; i++) {
            cJSON *line = cJSON_GetArrayItem(lines_obj, start_line + i);
            if (cJSON_IsString(line)) {
                mvprintw(i + 1, 0, "%s", line->valuestring);
            }
        }

        // Add a footer at the bottom
        attron(A_BOLD | COLOR_PAIR(2));
        mvprintw(max_y - 1, 0, "Press 'q' to exit | Use Arrow Keys to Scroll");
        attroff(A_BOLD | COLOR_PAIR(2));

        refresh();

        // Use the reusable input loop for scrolling and quitting
        if (handle_scrolling_and_quitting(&start_line, total_lines, display_lines)) {
            break; // Exit if 'q' is pressed
        }
    }

    // Free the JSON object
    cJSON_Delete(parsed_file);
}