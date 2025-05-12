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

        // Get user input
        ch = getch();
        switch (ch) {
            case KEY_UP:
                highlight = (highlight - 1 + NUM_OPTIONS) % NUM_OPTIONS; // Move up
                break;
            case KEY_DOWN:
                highlight = (highlight + 1) % NUM_OPTIONS; // Move down
                break;
            case '\n': // Enter key
                choice = highlight;
                break;
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
                print_wrapped(NUM_OPTIONS + 8, 0, "Press any key to return to the menu...");
                getch();
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

    int current_row = 6; // Start at row 6

    // Print the table header
    print_wrapped(current_row, 0, "%-20s %-20s %-20s", "Socket", "ID", "State");
    current_row += line_offset; // Adjust for the lines used
    print_wrapped(current_row, 0, "------------------------------------------------------------");
    current_row += line_offset; // Adjust for the lines used

    // Iterate through the array and print each bot's details
    cJSON *bot = NULL;
    int num_bots = 0;
    cJSON_ArrayForEach(bot, parsed_json) {
        cJSON *socket_obj = cJSON_GetObjectItem(bot, "socket");
        cJSON *id_obj = cJSON_GetObjectItem(bot, "id");
        cJSON *state_obj = cJSON_GetObjectItem(bot, "status");

        if (cJSON_IsString(socket_obj) && cJSON_IsString(id_obj) && cJSON_IsString(state_obj)) {
            print_wrapped(current_row++, 0, "%-20s %-20s %-20s",
                     socket_obj->valuestring,
                     id_obj->valuestring,
                     state_obj->valuestring);
        } else {
            attron(A_BOLD | COLOR_PAIR(4));
            print_wrapped(current_row++, 0, "Failed to parse bot details.");
            attroff(A_BOLD | COLOR_PAIR(4));
        }

        num_bots++;
    }

    if (num_bots == 0) {
            attron(A_BOLD | COLOR_PAIR(4));
            print_wrapped(current_row++, 0, "No bots are connected!");
            attroff(A_BOLD | COLOR_PAIR(4));
        }

    // Free the JSON object
    cJSON_Delete(parsed_json);
    refresh();
}

void get_file_from_bot() {
    char bot_id[256];
    char file_name[256];
    char post_data[512];
    char response[4096] = {0}; // Buffer to store the HTTP response

    // Prompt the user for the bot ID
    print_wrapped(2, 0, "Enter Bot ID [<IP>:<PORT>]: ");
    echo();
    getnstr(bot_id, sizeof(bot_id) - 1);
    noecho();

    // Validate the bot ID
    if (strlen(bot_id) == 0) {
        attron(A_BOLD | COLOR_PAIR(4));
        print_wrapped(4, 0, "Bot ID cannot be empty.");
        attroff(A_BOLD | COLOR_PAIR(4));
        refresh();
        return;
    }

    // Prompt the user for the file name
    print_wrapped(3, 0, "Enter File Name [ex: main.log]: ");
    echo();
    getnstr(file_name, sizeof(file_name) - 1);
    noecho();

    // Validate the file name
    if (strlen(file_name) == 0) {
        attron(A_BOLD | COLOR_PAIR(4));
        print_wrapped(5, 0, "File Name cannot be empty.");
        attroff(A_BOLD | COLOR_PAIR(4));
        refresh();
        return;
    }

    // Construct the POST data
    snprintf(post_data, sizeof(post_data), "bot_id=%s&file_name=%s", bot_id, file_name);

    CURL *curl = curl_easy_init();
    if (!curl) {
        attron(A_BOLD | COLOR_PAIR(4));
        print_wrapped(6, 0, "Failed to initialize CURL.");
        attroff(A_BOLD | COLOR_PAIR(4));
        refresh();
        return;
    }

    // Set up the CURL request
    curl_easy_setopt(curl, CURLOPT_URL, "http://127.0.0.1:8000/api/upload");
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        attron(A_BOLD | COLOR_PAIR(4));
        print_wrapped(7, 0, "Failed to send upload request: %s", curl_easy_strerror(res));
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
        print_wrapped(8, 0, "Failed to parse JSON response.");
        attroff(A_BOLD | COLOR_PAIR(4));
        refresh();
        return;
    }

    // Check for "status" or "error" in the response
    cJSON *status_obj = cJSON_GetObjectItem(parsed_json, "status");
    cJSON *error_obj = cJSON_GetObjectItem(parsed_json, "error");

    if (cJSON_IsString(status_obj)) {
        attron(A_BOLD | COLOR_PAIR(3));
        print_wrapped(9, 0, "Success!");
        attroff(A_BOLD | COLOR_PAIR(3));
    } else if (cJSON_IsString(error_obj)) {
        attron(A_BOLD | COLOR_PAIR(4));
        print_wrapped(9, 0, "Error: %s", error_obj->valuestring);
        attroff(A_BOLD | COLOR_PAIR(4));
    } else {
        attron(A_BOLD | COLOR_PAIR(4));
        print_wrapped(9, 0, "Unexpected response format.");
        attroff(A_BOLD | COLOR_PAIR(4));
    }

    // Free the JSON object
    cJSON_Delete(parsed_json);
    refresh();
}

void send_command_to_bot() {
    char bot_ids[1024] = {0};
    char num_clients_str[16] = {0};
    char cmd_id[64] = {0};
    char program[256] = {0};
    char params[256] = {0};
    char delay_str[16] = {0};
    char expected_code_str[16] = {0};
    char post_data[2048];
    char response[4096] = {0}; // Buffer to store the HTTP response

    int current_row = 2; // Start at row 2

    // Prompt the user for bot IDs
    print_wrapped(current_row, 0, "Enter Bot IDs (comma-separated, or leave empty to target random clients): ");
    current_row += line_offset; // Adjust for the lines used
    echo();
    getnstr(bot_ids, sizeof(bot_ids) - 1);
    noecho();

    // Prompt the user for the number of clients to target
    print_wrapped(current_row, 0, "Enter the number of clients to target (leave empty if using bot IDs): ");
    current_row += line_offset; // Adjust for the lines used
    echo();
    getnstr(num_clients_str, sizeof(num_clients_str) - 1);
    noecho();

    // Prompt the user for the command ID
    print_wrapped(current_row, 0, "Enter Command ID: ");
    current_row += line_offset; // Adjust for the lines used
    echo();
    getnstr(cmd_id, sizeof(cmd_id) - 1);
    noecho();

    // Prompt the user for the program to execute
    print_wrapped(current_row, 0, "Enter Program to Execute: ");
    current_row += line_offset; // Adjust for the lines used
    echo();
    getnstr(program, sizeof(program) - 1);
    noecho();

    // Prompt the user for parameters
    print_wrapped(current_row, 0, "Enter Parameters (space-separated, or leave empty): ");
    current_row += line_offset; // Adjust for the lines used
    echo();
    getnstr(params, sizeof(params) - 1);
    noecho();

    // Prompt the user for the delay
    print_wrapped(current_row, 0, "Enter Delay (in seconds, or leave empty for 0): ");
    current_row += line_offset; // Adjust for the lines used
    echo();
    getnstr(delay_str, sizeof(delay_str) - 1);
    noecho();

    // Prompt the user for the expected exit code
    print_wrapped(current_row, 0, "Enter Expected Exit Code (or leave empty for 0): ");
    current_row += line_offset; // Adjust for the lines used
    echo();
    getnstr(expected_code_str, sizeof(expected_code_str) - 1);
    noecho();

    // Construct the POST data
    snprintf(post_data, sizeof(post_data),
             "bot_ids=%s&num_clients=%s&cmd_id=%s&program=%s&params=%s&delay=%s&expected_code=%s",
             bot_ids, num_clients_str, cmd_id, program, params, delay_str, expected_code_str);

    CURL *curl = curl_easy_init();
    if (!curl) {
        attron(A_BOLD | COLOR_PAIR(4));
        print_wrapped(current_row, 0, "Failed to initialize CURL.");
        attroff(A_BOLD | COLOR_PAIR(4));
        current_row += line_offset; // Adjust for the lines used
        refresh();
        return;
    }

    // Set up the CURL request
    curl_easy_setopt(curl, CURLOPT_URL, "http://127.0.0.1:8000/api/command");
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        attron(A_BOLD | COLOR_PAIR(4));
        print_wrapped(current_row, 0, "Failed to send command: %s", curl_easy_strerror(res));
        attroff(A_BOLD | COLOR_PAIR(4));
        current_row += line_offset; // Adjust for the lines used
        refresh();
        curl_easy_cleanup(curl);
        return;
    }

    curl_easy_cleanup(curl);

    // Parse the JSON response using cJSON
    cJSON *parsed_json = cJSON_Parse(response);
    if (!parsed_json) {
        attron(A_BOLD | COLOR_PAIR(4));
        print_wrapped(current_row, 0, "Failed to parse JSON response.");
        attroff(A_BOLD | COLOR_PAIR(4));
        current_row += line_offset; // Adjust for the lines used
        refresh();
        return;
    }

    // Check for "status" or "error" in the response
    cJSON *status_obj = cJSON_GetObjectItem(parsed_json, "status");
    cJSON *error_obj = cJSON_GetObjectItem(parsed_json, "error");
    cJSON *targeted_clients_obj = cJSON_GetObjectItem(parsed_json, "targeted_clients");

    if (cJSON_IsString(status_obj)) {
        attron(A_BOLD | COLOR_PAIR(3));
        print_wrapped(current_row+1, 0, "Success: %s", status_obj->valuestring);
        attroff(A_BOLD | COLOR_PAIR(3));
        current_row += line_offset; // Adjust for the lines used
        if (cJSON_IsNumber(targeted_clients_obj)) {
            attron(A_BOLD | COLOR_PAIR(3));
            print_wrapped(current_row+1, 0, "Targeted Clients: %d", targeted_clients_obj->valueint);
            attroff(A_BOLD | COLOR_PAIR(3));
            current_row += line_offset; // Adjust for the lines used
        }
    } else if (cJSON_IsString(error_obj)) {
        attron(A_BOLD | COLOR_PAIR(4));
        print_wrapped(current_row, 0, "Error: %s", error_obj->valuestring);
        attroff(A_BOLD | COLOR_PAIR(4));
        current_row += line_offset; // Adjust for the lines used
    } else {
        attron(A_BOLD | COLOR_PAIR(4));
        print_wrapped(current_row, 0, "Unexpected response format.");
        attroff(A_BOLD | COLOR_PAIR(4));
        current_row += line_offset; // Adjust for the lines used
    }

    // Free the JSON object
    cJSON_Delete(parsed_json);
    refresh();
}

void get_bot_file() {
    char bot_id[256] = {0};
    char lines_str[16] = {0};
    char offset_str[16] = {0};
    char query_url[512];
    char response[4096] = {0}; // Buffer to store the HTTP response

    int current_row = 2; // Start at row 2

    // Prompt the user for the bot ID
    print_wrapped(current_row, 0, "Enter Bot ID (<ip>:<port>): ");
    current_row += line_offset; // Adjust for the lines used
    echo();
    getnstr(bot_id, sizeof(bot_id) - 1);
    noecho();

    // Validate the bot ID
    if (strlen(bot_id) == 0) {
        attron(A_BOLD | COLOR_PAIR(4));
        print_wrapped(current_row, 0, "Bot ID cannot be empty.");
        attroff(A_BOLD | COLOR_PAIR(4));
        current_row += line_offset; // Adjust for the lines used
        refresh();
        return;
    }

    // Prompt the user for the number of lines to fetch
    print_wrapped(current_row, 0, "Enter the number of lines to fetch: ");
    current_row += line_offset; // Adjust for the lines used
    echo();
    getnstr(lines_str, sizeof(lines_str) - 1);
    noecho();

    // Validate the number of lines
    if (strlen(lines_str) == 0 || atoi(lines_str) <= 0) {
        attron(A_BOLD | COLOR_PAIR(4));
        print_wrapped(current_row, 0, "Invalid number of lines. Please enter a positive integer.");
        attroff(A_BOLD | COLOR_PAIR(4));
        current_row += line_offset; // Adjust for the lines used
        refresh();
        return;
    }

    // Prompt the user for the offset
    print_wrapped(current_row, 0, "Enter the offset (number of most recent lines to ignore): ");
    current_row += line_offset; // Adjust for the lines used
    echo();
    getnstr(offset_str, sizeof(offset_str) - 1);
    noecho();

    // Validate the offset
    if (strlen(offset_str) == 0 || atoi(offset_str) < 0) {
        attron(A_BOLD | COLOR_PAIR(4));
        print_wrapped(current_row, 0, "Invalid offset. Please enter a non-negative integer.");
        attroff(A_BOLD | COLOR_PAIR(4));
        current_row += line_offset; // Adjust for the lines used
        refresh();
        return;
    }

    // Construct the query URL
    snprintf(query_url, sizeof(query_url),
             "http://127.0.0.1:8000/api/botfile?id=%s&lines=%s&offset=%s",
             bot_id, lines_str, offset_str);

    CURL *curl = curl_easy_init();
    if (!curl) {
        attron(A_BOLD | COLOR_PAIR(4));
        print_wrapped(current_row, 0, "Failed to initialize CURL.");
        attroff(A_BOLD | COLOR_PAIR(4));
        current_row += line_offset; // Adjust for the lines used
        refresh();
        return;
    }

    // Set up the CURL request
    curl_easy_setopt(curl, CURLOPT_URL, query_url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        attron(A_BOLD | COLOR_PAIR(4));
        print_wrapped(current_row, 0, "Failed to fetch bot file: %s", curl_easy_strerror(res));
        attroff(A_BOLD | COLOR_PAIR(4));
        current_row += line_offset; // Adjust for the lines used
        refresh();
        curl_easy_cleanup(curl);
        return;
    }

    curl_easy_cleanup(curl);

    // Parse the JSON response using cJSON
    cJSON *parsed_json = cJSON_Parse(response);
    if (!parsed_json) {
        attron(A_BOLD | COLOR_PAIR(4));
        print_wrapped(current_row, 0, "Failed to parse JSON response.");
        attroff(A_BOLD | COLOR_PAIR(4));
        current_row += line_offset; // Adjust for the lines used
        refresh();
        return;
    }

    // Extract the "lines" array from the response
    cJSON *lines_obj = cJSON_GetObjectItem(parsed_json, "lines");
    if (!cJSON_IsArray(lines_obj)) {
        attron(A_BOLD | COLOR_PAIR(4));
        print_wrapped(current_row, 0, "Unexpected response format. 'lines' array not found.");
        attroff(A_BOLD | COLOR_PAIR(4));
        current_row += line_offset; // Adjust for the lines used
        cJSON_Delete(parsed_json);
        refresh();
        return;
    }

    // Display the lines in the CLI
    attron(A_BOLD | COLOR_PAIR(3));
    print_wrapped(current_row, 0, "Bot file content:");
    attroff(A_BOLD | COLOR_PAIR(3));
    current_row += line_offset; // Adjust for the lines used
    cJSON *line = NULL;
    cJSON_ArrayForEach(line, lines_obj) {
        if (cJSON_IsString(line)) {
            print_wrapped(current_row, 0, "%s", line->valuestring);
            current_row += line_offset; // Adjust for the lines used
        }
    }

    // Free the JSON object
    cJSON_Delete(parsed_json);
    refresh();
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
    int input_lines = calculate_input_lines(buffer, start_col, max_x);
    line_offset += input_lines; // Add the input lines to the global line offset
}