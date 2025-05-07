// Burak Erdilli 19011046
// Revised, fixed, improved, and optimized version

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

// Structure to hold run-length encoded data
typedef struct {
    int width;
    int height;
    int num_pairs; // Number of count-value pairs
    int *data;     // Array storing pairs: [count1, value1, count2, value2, ...]
} EncodedData;

// Function Prototypes
bool read_pgm_header(FILE* fp, int* width, int* height, int* max_val);
EncodedData* encode_pgm(FILE* fp, int width, int height, int max_val);
bool write_encoded_data(const char* filename, const EncodedData* encoded_data);
EncodedData* load_encoded_data(const char* filename);
bool decode_pgm(const EncodedData* encoded_data, const char* output_filename);
void change_pixel_color(EncodedData* encoded_data, int oldPixel, int newPixel);
void print_histogram(const EncodedData* encoded_data);
void free_encoded_data(EncodedData* encoded_data);

// --- Helper Functions ---

// Reads PGM (P2) header information
// Returns true on success, false on failure or if not P2 format.
bool read_pgm_header(FILE* fp, int* width, int* height, int* max_val) {
    char image_type[3]; // Should be "P2"
    int read_count;

    // Read PGM type (P2), width, height, and max_val
    read_count = fscanf(fp, "%2s %d %d %d", image_type, width, height, max_val);

    if (read_count == 4 && strcmp(image_type, "P2") == 0) {
        // Skip any remaining whitespace after max_val
        char c;
        while ((c = fgetc(fp)) != EOF && (c == ' ' || c == '\t' || c == '\n' || c == '\r'));
        if (c != EOF) ungetc(c, fp); // Put non-whitespace character back

        printf("\nHeader information of the file: \n\n");
        printf("%s %d %d %d\n", image_type, *width, *height, *max_val);
        return true;
    } else {
        fprintf(stderr, "Error: Invalid PGM header or unsupported format. Expected P2.\n");
        // File pointer needs to be closed by the caller if this function returns false
        return false;
    }
}

// Encodes PGM pixel data using RLE
// Returns dynamically allocated EncodedData on success, NULL on failure.
EncodedData* encode_pgm(FILE* fp, int width, int height, int max_val) {
    if (!fp) return NULL;

    // Use a dynamic array (like a vector in C++) by reallocating
    int* temp_data = NULL;
    int current_capacity = 0;
    int current_size = 0; // Number of integers (pairs * 2 + 2 for w/h)

    int element1, element2;
    int index = 0; // run length

    // Allocate initial capacity (can be arbitrary, will grow)
    current_capacity = 10; // Start with space for 10 pairs + w/h
    temp_data = (int*)malloc(current_capacity * 2 * sizeof(int) + 2 * sizeof(int));
    if (!temp_data) {
        perror("Error allocating memory for encoding");
        return NULL;
    }

    // Store width and height at the beginning
    temp_data[0] = width;
    temp_data[1] = height;
    current_size = 2;

    // Read the first pixel value outside the loop
    if (fscanf(fp, "%d", &element2) == 1) {
        index = 1; // Start run length for the first pixel
    } else {
        // Handle case of empty pixel data (shouldn't happen in valid PGM)
        fprintf(stderr, "Error: Could not read first pixel value.\n");
        free(temp_data);
        return NULL;
    }

    // Read subsequent pixel values
    while (fscanf(fp, "%d", &element1) == 1) {
        if (element1 == element2) {
            index++;
        } else {
            // End of a run, store the pair (index, element2)
            if (current_size + 2 > current_capacity * 2 + 2) {
                current_capacity *= 2; // Double capacity
                int* new_data = (int*)realloc(temp_data, current_capacity * 2 * sizeof(int) + 2 * sizeof(int));
                if (!new_data) {
                    perror("Error reallocating memory during encoding");
                    free(temp_data);
                    return NULL;
                }
                temp_data = new_data;
            }
            temp_data[current_size++] = index;     // Store count
            temp_data[current_size++] = element2;   // Store value

            index = 1; // Start new run with element1
            element2 = element1;
        }
    }

    // Store the last run
    if (current_size + 2 > current_capacity * 2 + 2) {
         current_capacity += 2; // Add just enough for the last pair
         int* new_data = (int*)realloc(temp_data, current_capacity * 2 * sizeof(int) + 2 * sizeof(int));
         if (!new_data) {
             perror("Error reallocating memory during encoding");
             free(temp_data);
             return NULL;
         }
         temp_data = new_data;
    }
    temp_data[current_size++] = index;     // Store count
    temp_data[current_size++] = element2;   // Store value // element2 holds the last pixel value

    // Create the final EncodedData structure
    EncodedData* encoded_data = (EncodedData*)malloc(sizeof(EncodedData));
    if (!encoded_data) {
         perror("Error allocating memory for EncodedData structure");
         free(temp_data);
         return NULL;
    }

    encoded_data->width = width;
    encoded_data->height = height;
    encoded_data->num_pairs = (current_size - 2) / 2; // Total integers minus w/h, divided by 2
    encoded_data->data = (int*)realloc(temp_data, current_size * sizeof(int)); // Trim to actual size
     if (!encoded_data->data && current_size > 0) { // realloc can return NULL if size > 0 and fails
        perror("Error trimming memory after encoding");
        free(temp_data); // free the original pointer if realloc failed
        free(encoded_data);
        return NULL;
    } else if (!encoded_data->data && current_size == 0 && temp_data != NULL) {
         // Special case: If size is 0, realloc(temp_data, 0) might return NULL or non-NULL.
         // If it's NULL and temp_data was not NULL, we might lose the original pointer if not careful.
         // But given the logic, current_size will always be at least 4 (w, h, count1, value1).
         // So this else if branch for size == 0 is likely not needed but added for robustness.
         free(temp_data); // Ensure original temp_data is freed if realloc(0) fails and temp_data was valid.
         encoded_data->data = NULL; // Set data to NULL as there is no data
    }


    printf("\nEncoding complete. Encoded data size: %d integers (%d pairs)\n", current_size, encoded_data->num_pairs);

    return encoded_data;
}

// Writes encoded data to a text file
bool write_encoded_data(const char* filename, const EncodedData* encoded_data) {
    if (!encoded_data || !encoded_data->data) {
        fprintf(stderr, "Error: No encoded data to write.\n");
        return false;
    }

    FILE* file = fopen(filename, "w+"); // Use w+ to create/truncate and allow reading back if needed
    if (!file) {
        perror("Error opening file for writing encoded data");
        return false;
    }

    // Write width and height first
    fprintf(file, "%d %d ", encoded_data->width, encoded_data->height);

    // Write count-value pairs
    for (int i = 0; i < encoded_data->num_pairs * 2; i += 2) {
        fprintf(file, "%d %d ", encoded_data->data[i + 2], encoded_data->data[i + 3]); // data[0]/[1] are w/h, pairs start at index 2
    }
    fprintf(file, "\n"); // Add a newline at the end

    if (fclose(file) == EOF) {
        perror("Error closing encoded file after writing");
        return false;
    }

    printf("Encoded data written to %s\n", filename);
    return true;
}


// Loads encoded data from a text file into EncodedData structure
// Returns dynamically allocated EncodedData on success, NULL on failure.
EncodedData* load_encoded_data(const char* filename) {
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        perror("Error opening encoded file for reading");
        return NULL;
    }

    int width, height;
    // Read width and height first
    if (fscanf(fp, "%d %d", &width, &height) != 2) {
        fprintf(stderr, "Error reading width and height from encoded file.\n");
        fclose(fp);
        return NULL;
    }

    // Read the rest of the integers (count-value pairs) dynamically
    int* temp_data = NULL;
    int current_capacity = 0;
    int current_size = 0; // Number of integers read (excluding w/h initially)

    int value;
    while (fscanf(fp, "%d", &value) == 1) {
        if (current_size >= current_capacity) {
            current_capacity = (current_capacity == 0) ? 10 : current_capacity * 2;
            int* new_data = (int*)realloc(temp_data, current_capacity * sizeof(int));
            if (!new_data) {
                perror("Error reallocating memory during loading encoded data");
                free(temp_data);
                fclose(fp);
                return NULL;
            }
            temp_data = new_data;
        }
        temp_data[current_size++] = value;
    }

    fclose(fp); // Close the file after reading

    if (current_size % 2 != 0) {
        fprintf(stderr, "Warning: Encoded file has an odd number of data values after width/height. File may be corrupt.\n");
        // Decide whether to fail or continue. For now, we'll continue but warn.
    }

    // Create the final EncodedData structure
    EncodedData* encoded_data = (EncodedData*)malloc(sizeof(EncodedData));
    if (!encoded_data) {
         perror("Error allocating memory for EncodedData structure");
         free(temp_data);
         return NULL;
    }

    encoded_data->width = width;
    encoded_data->height = height;
    encoded_data->num_pairs = current_size / 2;
    encoded_data->data = (int*)realloc(temp_data, current_size * sizeof(int)); // Trim to actual size
    if (!encoded_data->data && current_size > 0) { // realloc can return NULL if size > 0 and fails
        perror("Error trimming memory after loading encoded data");
        free(temp_data); // free the original pointer if realloc failed
        free(encoded_data);
        return NULL;
    } else if (!encoded_data->data && current_size == 0 && temp_data != NULL) {
         free(temp_data);
         encoded_data->data = NULL;
    }


    printf("Encoded data loaded from %s. Width: %d, Height: %d, %d pairs.\n", filename, width, height, encoded_data->num_pairs);

    return encoded_data;
}


// Decodes encoded data and writes to a PGM (P2) file
bool decode_pgm(const EncodedData* encoded_data, const char* output_filename) {
    if (!encoded_data || !encoded_data->data || encoded_data->num_pairs == 0) {
        fprintf(stderr, "Error: No encoded data to decode.\n");
        return false;
    }

    // Basic checks on encoded data integrity
    long long total_pixels_sum = 0;
    for (int i = 0; i < encoded_data->num_pairs * 2; i += 2) {
        int count = encoded_data->data[i];
        int value = encoded_data->data[i + 1];
        if (count <= 0) {
             fprintf(stderr, "Error: Invalid count (%d) found in encoded data.\n", count);
             return false;
        }
        if (value < 0 || value > 255) { // Assuming 8-bit PGM (max 255)
             fprintf(stderr, "Error: Invalid pixel value (%d) found in encoded data.\n", value);
             return false;
        }
        total_pixels_sum += count;
        if (i + 2 < encoded_data->num_pairs * 2) { // Check if consecutive values are different (RLE property)
            if (value == encoded_data->data[i + 3]) {
                fprintf(stderr, "Error: Invalid encoded data. Consecutive runs have the same pixel value (%d).\n", value);
                return false;
            }
        }
    }

    if (total_pixels_sum != (long long)encoded_data->width * encoded_data->height) {
        fprintf(stderr, "Error: Total pixel count from encoded data (%lld) does not match header size (%d x %d = %d).\n",
                total_pixels_sum, encoded_data->width, encoded_data->height, encoded_data->width * encoded_data->height);
        return false;
    }

    printf("\nAll file controls successfully passed! Starting to Decode...\n\n");

    FILE* file = fopen(output_filename, "w+");
    if (!file) {
        perror("Error opening file for writing decoded PGM");
        return false;
    }

    // Write PGM P2 header (assuming 255 max value as per original code)
    fprintf(file, "P2\n%d %d\n255\n", encoded_data->width, encoded_data->height);

    int pixels_on_current_line = 0;
    for (int i = 0; i < encoded_data->num_pairs * 2; i += 2) {
        int count = encoded_data->data[i];
        int value = encoded_data->data[i + 1];

        for (int j = 0; j < count; ++j) {
            fprintf(file, "%d", value);
            pixels_on_current_line++;

            if (pixels_on_current_line == encoded_data->width) {
                fprintf(file, "\n"); // Newline after width pixels
                pixels_on_current_line = 0;
            } else {
                fprintf(file, " "); // Space between pixel values on the same line
            }
        }
    }

    // Ensure file ends with a newline if the last line was full
    if (pixels_on_current_line != 0) {
         fprintf(file, "\n");
    }


    if (fclose(file) == EOF) {
        perror("Error closing decoded PGM file after writing");
        return false;
    }

    printf("Finished Decoding! %s File is Available now!\n\n", output_filename);
    return true;
}


// Changes all occurrences of a specific pixel color value in the encoded data
void change_pixel_color(EncodedData* encoded_data, int oldPixel, int newPixel) {
    if (!encoded_data || !encoded_data->data) {
        fprintf(stderr, "Error: No encoded data loaded.\n");
        return;
    }

    if (oldPixel < 0 || oldPixel > 255 || newPixel < 0 || newPixel > 255) {
        fprintf(stderr, "Error: Pixel values must be between 0 and 255.\n");
        return;
    }

    bool changed = false;
    // Iterate through count-value pairs (data starts at index 0 now in the data array)
    for (int i = 0; i < encoded_data->num_pairs * 2; i += 2) {
        if (encoded_data->data[i + 1] == oldPixel) {
            encoded_data->data[i + 1] = newPixel;
            changed = true;
        }
    }

    if (changed) {
        printf("\nSelected color %d is changed to %d!\n", oldPixel, newPixel);
        // Note: The change is made in memory. You need to call write_encoded_data
        // separately to save the changes back to the file.
        printf("Changes made in memory. Use the save option to write to file.\n");
    } else {
        printf("\nError: The color %d is not found in the encoded data.\n", oldPixel);
    }
}

/*
// --- Change Specific Pixel (Needs significant work) ---
// This function was commented out in the original code.
// Modifying a single pixel in Run-Length Encoded data is complex
// because it might require splitting an existing run into two or three runs,
// or merging adjacent runs if the new pixel value matches neighbours.
//
// Example:
// Original RLE: ... 100 50 ... (100 pixels of value 50)
// Change pixel at position 5 in this run to value 60
// Resulting RLE: ... 5 50 1 60 94 50 ... (Splits the original run)
//
// Example 2:
// Original RLE: ... 5 50 10 60 8 50 ...
// Change pixel at the end of the first run (pos 5) to value 60
// Resulting RLE: ... 4 50 15 60 8 50 ... (Merges two runs)
//
// The commented-out implementation attempted to manipulate indices and values
// within the flattened array, which is prone to errors when runs need splitting
// or merging. A robust implementation would likely need to create a new list
// of RLE pairs by iterating through the original data, applying the change at
// the correct logical position (which first requires calculating the position
// within the flattened RLE data), and generating the new sequence of runs.
//
// Due to its complexity and incomplete state, this function is left commented out.
void changeSpecificPixel(EncodedData* encoded_data, int pos_x, int pos_y, int value) {
    // Implementation would go here if fixed
    printf("\nChange specific pixel functionality is complex and not implemented.\n");
}
*/


// Prints a histogram of pixel values in the encoded data
void print_histogram(const EncodedData* encoded_data) {
    if (!encoded_data || !encoded_data->data || encoded_data->num_pairs == 0) {
        fprintf(stderr, "Error: No encoded data loaded for histogram.\n");
        return;
    }

    long long histogram[256] = {0}; // Use long long for counts
    long long total_pixels = (long long)encoded_data->width * encoded_data->height;

    // Iterate through count-value pairs
    for (int i = 0; i < encoded_data->num_pairs * 2; i += 2) {
        int count = encoded_data->data[i];
        int value = encoded_data->data[i + 1];
        if (value >= 0 && value <= 255) {
             histogram[value] += count;
        }
    }

    printf("\n--- Pixel Color Histogram ---\n");
    for (int i = 0; i < 256; ++i) {
        if (histogram[i] > 0) {
            double ratio = (double)histogram[i] / total_pixels * 100.0;
            printf("Color: %d, Count: %lld, Ratio: %.2f%%\n", i, histogram[i], ratio);
        }
    }
    printf("-----------------------------\n");

    // Original code appended to test_encoded.txt - removed as likely unintended.
    // If file output is needed, open a separate file (e.g., test_histogram.txt)
}

// Frees the memory allocated for the EncodedData structure
void free_encoded_data(EncodedData* encoded_data) {
    if (encoded_data) {
        free(encoded_data->data); // Free the data array
        free(encoded_data);       // Free the structure itself
    }
}

// --- Main Function ---

int main() {
    bool exit_program = false;
    int selection;
    char filename[256]; // Increased buffer size for filename
    FILE* fp = NULL;
    EncodedData* current_encoded_data = NULL; // Pointer to currently loaded encoded data

    printf("\n------WELCOME------\n");

    while (!exit_program) {
        // Free previously loaded data if any, before new main menu selection
        free_encoded_data(current_encoded_data);
        current_encoded_data = NULL;

        printf("\nPlease select an option: (ex: 1,2,3)\n");
        printf("\n1) File Encoding\n2) File Decoding\n3) Exit\n"); // Simplified main menu
        printf("\nSelection: ");

        if (scanf("%d", &selection) != 1) {
             fprintf(stderr, "Invalid input. Please enter a number.\n");
             // Clear input buffer
             while (getchar() != '\n');
             continue; // Restart loop
        }

        switch (selection) {
            case 1: // File Encoding
                printf("\nPlease type the input PGM file name (.pgm): ");
                if (scanf("%s", filename) != 1) {
                    fprintf(stderr, "Error reading filename.\n");
                    break;
                }
                fp = fopen(filename, "r");
                if (!fp) {
                    perror("Error opening input PGM file");
                    break;
                }

                int width, height, max_val;
                if (read_pgm_header(fp, &width, &height, &max_val)) {
                    printf("\nEncoding the file...\n");
                    // Pass fp directly for encoding
                    EncodedData* encoded_result = encode_pgm(fp, width, height, max_val);
                    fclose(fp); // Close input file after reading

                    if (encoded_result) {
                        printf("Encoding process completed.\n");
                        // Automatically write the encoded data to a default file
                        if (write_encoded_data("test_encoded.txt", encoded_result)) {
                            printf("Encoded data saved to test_encoded.txt.\n");
                            // Load it into current_encoded_data for sub-menu operations
                            current_encoded_data = encoded_result; // Keep the data in memory
                            encoded_result = NULL; // Avoid double free
                        } else {
                            // Writing failed, free the encoded data
                            free_encoded_data(encoded_result);
                            encoded_result = NULL;
                        }

                        // --- Encoded File Operations Sub-menu ---
                        if (current_encoded_data) {
                            int sub_selection;
                             do {
                                printf("\n\nPlease select an option for test_encoded.txt (in memory): \n");
                                printf("\n1) Change Pixel Colors\n2) Print Histogram\n3) Save Encoded Data to File\n0) Return to Main Menu\n"); // Removed Change Pixel as it's complex
                                printf("\nSelection: ");

                                if (scanf("%d", &sub_selection) != 1) {
                                    fprintf(stderr, "Invalid input. Please enter a number.\n");
                                    // Clear input buffer
                                    while (getchar() != '\n');
                                    continue; // Restart sub-menu loop
                                }

                                switch (sub_selection) {
                                    case 1: // Change Pixel Colors
                                        { // Use a block to scope variables
                                            int old_color, new_color;
                                            printf("Enter color to change (0-255): ");
                                            if (scanf("%d", &old_color) != 1) {
                                                fprintf(stderr, "Invalid input.\n");
                                                while (getchar() != '\n');
                                                break;
                                            }
                                            printf("Enter new color (0-255): ");
                                             if (scanf("%d", &new_color) != 1) {
                                                fprintf(stderr, "Invalid input.\n");
                                                while (getchar() != '\n');
                                                break;
                                            }
                                            change_pixel_color(current_encoded_data, old_color, new_color);
                                            // Changes are in memory, need to save later
                                        }
                                        break;
                                    case 2: // Print Histogram
                                        print_histogram(current_encoded_data);
                                        break;
                                    case 3: // Save Encoded Data
                                         {
                                             char output_filename[256];
                                             printf("Enter filename to save encoded data to: ");
                                              if (scanf("%s", output_filename) != 1) {
                                                fprintf(stderr, "Error reading filename.\n");
                                                break;
                                            }
                                             if (write_encoded_data(output_filename, current_encoded_data)) {
                                                printf("Encoded data saved successfully.\n");
                                             }
                                         }
                                         break;
                                    case 0: // Return
                                        printf("\nReturning to Main Menu...\n");
                                        break;
                                    default:
                                        printf("\nInvalid selection. Please try again.\n");
                                }
                            } while (sub_selection != 0);
                        } // if (current_encoded_data)
                    } // if (encoded_result)
                    // else encoding failed, error message already printed
                } else {
                     // Header read failed, error message already printed
                     fclose(fp); // Ensure file is closed on header failure
                }
                break;

            case 2: // File Decoding
                printf("\nPlease type the input encoded file name (.txt): ");
                 if (scanf("%s", filename) != 1) {
                    fprintf(stderr, "Error reading filename.\n");
                    break;
                }
                current_encoded_data = load_encoded_data(filename); // Load encoded data
                if (current_encoded_data) {
                     char output_filename[256];
                     printf("Enter output PGM file name (.pgm): ");
                      if (scanf("%s", output_filename) != 1) {
                        fprintf(stderr, "Error reading filename.\n");
                        free_encoded_data(current_encoded_data);
                        current_encoded_data = NULL;
                        break;
                    }
                    // Decode and write the PGM file
                    decode_pgm(current_encoded_data, output_filename);
                }
                // Memory for current_encoded_data will be freed at the start of the next main loop iteration
                break;

            case 3: // Exit
                printf("\nProgram Closing...\n");
                exit_program = true;
                break;

            default:
                printf("\nInvalid selection. Please try again.\n");
        }
    }

    // Final cleanup if user exited without going through main loop cleanup
    free_encoded_data(current_encoded_data);

    return 0;
}