#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <direct.h>
#include <windows.h>
#include <zip.h>

#define MAX_LINE 2048

// Log levels
typedef enum {
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR
} LogLevel;

// Logging function
void log_message(LogLevel level, const char* format, ...) {
    time_t now;
    struct tm timeinfo;
    char timestamp[20];
    va_list args;
    
    // Get current time
    time(&now);
    localtime_s(&timeinfo, &now);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &timeinfo);
    
    // Print timestamp and level
    switch(level) {
        case LOG_INFO:
            printf("[%s] INFO: ", timestamp);
            break;
        case LOG_WARNING:
            printf("[%s] WARNING: ", timestamp);
            break;
        case LOG_ERROR:
            fprintf(stderr, "[%s] ERROR: ", timestamp);
            break;
    }
    
    // Print the actual message
    va_start(args, format);
    if (level == LOG_ERROR) {
        vfprintf(stderr, format, args);
        fprintf(stderr, "\n");
    } else {
        vprintf(format, args);
        printf("\n");
    }
    va_end(args);
}

// Windows-compatible basename function
char* win_basename(char* path) {
    char* base = strrchr(path, '\\');
    if (!base) base = strrchr(path, '/');
    return base ? base + 1 : path;
}

void create_metadata_file(const char *pdf_path, const char *output_dir) {
    struct stat file_info;
    char txt_path[MAX_PATH];
    char base_name[MAX_PATH];
    char *pdf_name;
    FILE *txt_file;
    char date_str[26];
    
    // Get file information
    if (stat(pdf_path, &file_info) != 0) {
        log_message(LOG_ERROR, "Failed to get file info for: %s", pdf_path);
        return;
    }
    
    // Get base name of PDF file
    strcpy(base_name, pdf_path);
    pdf_name = win_basename(base_name);
    
    // Create txt filename
    snprintf(txt_path, sizeof(txt_path), "%s\\%.*s.txt", 
             output_dir, (int)(strlen(pdf_name) - 4), pdf_name);
    
    // Open txt file for writing
    txt_file = fopen(txt_path, "w");
    if (!txt_file) {
        log_message(LOG_ERROR, "Failed to create metadata file: %s", txt_path);
        return;
    }
    
    // Convert time to string without newline
    ctime_s(date_str, sizeof(date_str), &file_info.st_mtime);
    date_str[24] = '\0';  // Remove newline
    
    // Write metadata line: filename|size|date
    fprintf(txt_file, "%s|%ld|%s", 
            pdf_name,
            file_info.st_size,
            date_str);
    
    fclose(txt_file);
    log_message(LOG_INFO, "Created metadata file: %s", txt_path);
}

int main(int argc, char *argv[]) {
    struct zip *zip_file;
    struct zip_stat zip_stats;
    char *buffer;
    int err;
    zip_uint64_t i;
    char output_dir[MAX_PATH];
    char pdf_path[MAX_PATH];
    FILE *pdf_file;
    char normalized_path[MAX_PATH];
    int pdf_count = 0;
    int success_count = 0;
    
    // Check arguments
    if (argc != 2) {
        log_message(LOG_ERROR, "Usage: %s <zip_file_path>", argv[0]);
        return 1;
    }
    
    log_message(LOG_INFO, "Starting PDF processor...");
    log_message(LOG_INFO, "Processing ZIP file: %s", argv[1]);
    
    // Normalize path (handle both forward and back slashes)
    char *p;
    strcpy(normalized_path, argv[1]);
    for (p = normalized_path; *p; p++) {
        if (*p == '/') *p = '\\';
    }
    
    // Open zip file
    zip_file = zip_open(normalized_path, 0, &err);
    if (!zip_file) {
        log_message(LOG_ERROR, "Failed to open ZIP file (error code: %d)", err);
        return 1;
    }
    
    // Create output directory (same name as zip file without .zip extension)
    snprintf(output_dir, sizeof(output_dir), "%.*s", 
             (int)(strlen(normalized_path) - 4), normalized_path);
    if (_mkdir(output_dir) == 0) {
        log_message(LOG_INFO, "Created output directory: %s", output_dir);
    } else if (errno != EEXIST) {
        log_message(LOG_ERROR, "Failed to create output directory: %s", output_dir);
        zip_close(zip_file);
        return 1;
    }
    
    // Process each file in the zip
    zip_uint64_t total_files = zip_get_num_entries(zip_file, 0);
    log_message(LOG_INFO, "Found %llu files in ZIP archive", total_files);
    
    for (i = 0; i < total_files; i++) {
        if (zip_stat_index(zip_file, i, 0, &zip_stats) == 0) {
            // Check if file is a PDF
            if (strlen(zip_stats.name) > 4 && 
                _stricmp(zip_stats.name + strlen(zip_stats.name) - 4, ".pdf") == 0) {
                
                pdf_count++;
                log_message(LOG_INFO, "Processing PDF %d: %s", pdf_count, zip_stats.name);
                
                struct zip_file *zf = zip_fopen_index(zip_file, i, 0);
                if (!zf) {
                    log_message(LOG_ERROR, "Failed to open file in ZIP: %s", zip_stats.name);
                    continue;
                }
                
                // Create path for extracted PDF
                snprintf(pdf_path, sizeof(pdf_path), "%s\\%s", 
                         output_dir, win_basename((char*)zip_stats.name));
                
                // Extract PDF file
                pdf_file = fopen(pdf_path, "wb");
                if (!pdf_file) {
                    log_message(LOG_ERROR, "Failed to create PDF file: %s", pdf_path);
                    zip_fclose(zf);
                    continue;
                }
                
                // Allocate buffer for reading
                buffer = malloc(zip_stats.size);
                if (!buffer) {
                    log_message(LOG_ERROR, "Memory allocation failed for file: %s", zip_stats.name);
                    fclose(pdf_file);
                    zip_fclose(zf);
                    continue;
                }
                
                // Read and write PDF content
                if (zip_fread(zf, buffer, zip_stats.size) == zip_stats.size) {
                    fwrite(buffer, 1, zip_stats.size, pdf_file);
                    fclose(pdf_file);
                    
                    log_message(LOG_INFO, "Successfully extracted: %s", pdf_path);
                    
                    // Create metadata text file
                    create_metadata_file(pdf_path, output_dir);
                    success_count++;
                } else {
                    log_message(LOG_ERROR, "Failed to read ZIP file content for: %s", zip_stats.name);
                    fclose(pdf_file);
                }
                
                free(buffer);
                zip_fclose(zf);
            }
        }
    }
    
    zip_close(zip_file);
    
    // Print summary
    log_message(LOG_INFO, "Processing complete!");
    log_message(LOG_INFO, "Summary:");
    log_message(LOG_INFO, "  Total files in ZIP: %llu", total_files);
    log_message(LOG_INFO, "  PDFs found: %d", pdf_count);
    log_message(LOG_INFO, "  Successfully processed: %d", success_count);
    if (pdf_count != success_count) {
        log_message(LOG_WARNING, "  Failed to process %d PDF files", pdf_count - success_count);
    }
    
    return 0;
}