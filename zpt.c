#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <direct.h>
#include <zip.h>
#include <windows.h>

#define MAX_PATH 1024
#define MAX_LINE 2048

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
        fprintf(stderr, "Error getting file info for: %s\n", pdf_path);
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
        fprintf(stderr, "Error creating text file: %s\n", txt_path);
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
    
    // Check arguments
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <zip_file_path>\n", argv[0]);
        return 1;
    }
    
    // Normalize path (handle both forward and back slashes)
    char *p;
    strcpy(normalized_path, argv[1]);
    for (p = normalized_path; *p; p++) {
        if (*p == '/') *p = '\\';
    }
    
    // Open zip file
    zip_file = zip_open(normalized_path, 0, &err);
    if (!zip_file) {
        fprintf(stderr, "Error opening zip file: %d\n", err);
        return 1;
    }
    
    // Create output directory (same name as zip file without .zip extension)
    snprintf(output_dir, sizeof(output_dir), "%.*s", 
             (int)(strlen(normalized_path) - 4), normalized_path);
    _mkdir(output_dir);  // Windows mkdir
    
    // Process each file in the zip
    for (i = 0; i < zip_get_num_entries(zip_file, 0); i++) {
        if (zip_stat_index(zip_file, i, 0, &zip_stats) == 0) {
            // Check if file is a PDF
            if (strlen(zip_stats.name) > 4 && 
                _stricmp(zip_stats.name + strlen(zip_stats.name) - 4, ".pdf") == 0) {
                
                struct zip_file *zf = zip_fopen_index(zip_file, i, 0);
                if (!zf) {
                    fprintf(stderr, "Error opening file in zip: %s\n", zip_stats.name);
                    continue;
                }
                
                // Create path for extracted PDF
                snprintf(pdf_path, sizeof(pdf_path), "%s\\%s", 
                         output_dir, win_basename((char*)zip_stats.name));
                
                // Extract PDF file
                pdf_file = fopen(pdf_path, "wb");
                if (!pdf_file) {
                    fprintf(stderr, "Error creating PDF file: %s\n", pdf_path);
                    zip_fclose(zf);
                    continue;
                }
                
                // Allocate buffer for reading
                buffer = malloc(zip_stats.size);
                if (!buffer) {
                    fprintf(stderr, "Error allocating memory\n");
                    fclose(pdf_file);
                    zip_fclose(zf);
                    continue;
                }
                
                // Read and write PDF content
                if (zip_fread(zf, buffer, zip_stats.size) == zip_stats.size) {
                    fwrite(buffer, 1, zip_stats.size, pdf_file);
                    fclose(pdf_file);
                    
                    // Create metadata text file
                    create_metadata_file(pdf_path, output_dir);
                } else {
                    fprintf(stderr, "Error reading ZIP file content\n");
                }
                
                free(buffer);
                zip_fclose(zf);
            }
        }
    }
    
    zip_close(zip_file);
    return 0;
}