/*
 * ARX Virtual Machine
 * Executes compiled ARX programs (.arxmod files)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "runtime/runtime.h"

// Global debug flag
bool debug_mode = false;

// Command line options
typedef struct {
    bool debug_mode;
    bool trace_execution;
    bool dump_state;
    bool step_mode;
    const char *input_file;
    const char *output_file;
} vm_options_t;

// Function prototypes
void print_usage(const char* program_name);
void print_version(void);
bool parse_arguments(int argc, char *argv[], vm_options_t *options);
void print_vm_info(void);

int main(int argc, char *argv[])
{
    vm_options_t options = {0};
    
    // Parse command line arguments
    if (!parse_arguments(argc, argv, &options)) {
        printf("Main: Failed to parse arguments\n");
        return 1;
    }
    
    // Set global debug mode
    debug_mode = options.debug_mode;
    
    if (options.debug_mode) {
        printf("Main: Starting ARX VM\n");
        printf("Main: Parsing command line arguments\n");
        printf("Main: Initializing runtime\n");
        print_vm_info();
    }
    runtime_context_t runtime;
    runtime_config_t config = RUNTIME_CONFIG_DEFAULT;
    config.debug_mode = options.debug_mode;
    config.trace_execution = options.trace_execution;
    config.dump_state_on_error = true;
    
    if (!runtime_init(&runtime, &config)) {
        printf("Error: Failed to initialize ARX VM runtime\n");
        return 1;
    }
    
    // Load program
    if (!runtime_load_program(&runtime, options.input_file)) {
        printf("Error: Failed to load program '%s'\n", options.input_file);
        runtime_cleanup(&runtime);
        return 1;
    }
    
    // Dump initial state if requested
    if (options.dump_state) {
        runtime_dump_state(&runtime);
    }
    
    // Execute program
    bool success = false;
    if (options.step_mode) {
        printf("Step mode: Press Enter to execute next instruction, 'q' to quit\n");
        char input[256];
        while (!runtime.vm.halted && runtime.vm.pc < runtime.vm.instruction_count) {
            if (fgets(input, sizeof(input), stdin) != NULL) {
                if (input[0] == 'q' || input[0] == 'Q') {
                    break;
                }
                if (!runtime_step(&runtime)) {
                    printf("Step execution failed: %s\n", 
                           runtime_get_error_string(&runtime));
                    break;
                }
            }
        }
        success = runtime.vm.halted;
    } else {
        if (options.debug_mode) {
            printf("Main: About to call runtime_execute\n");
        }
        success = runtime_execute(&runtime);
        if (options.debug_mode) {
            printf("Main: runtime_execute returned %s\n", success ? "true" : "false");
        }
    }
    
    // Dump final state if requested
    if (options.dump_state) {
        runtime_dump_state(&runtime);
    }
    
    // Cleanup
    runtime_cleanup(&runtime);
    
    if (success) {
        if (options.debug_mode) {
            printf("Program executed successfully\n");
        }
        return 0;
    } else {
        printf("Program execution failed: %s\n", 
               vm_error_to_string(runtime_get_last_error(&runtime)));
        return 1;
    }
}

void print_usage(const char* program_name)
{
    printf("ARX Virtual Machine v1.0\n");
    printf("Usage: %s [options] <arxmod_file>\n", program_name);
    printf("\n");
    printf("Options:\n");
    printf("  -debug          Enable debug output\n");
    printf("  -trace          Trace instruction execution\n");
    printf("  -dump           Dump VM state before and after execution\n");
    printf("  -step           Step through execution interactively\n");
    printf("  -o <file>       Output file (not used yet)\n");
    printf("  -h, --help      Show this help message\n");
    printf("  -v, --version   Show version information\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s program.arxmod\n", program_name);
    printf("  %s -debug -trace program.arxmod\n", program_name);
    printf("  %s -step program.arxmod\n", program_name);
    printf("  %s -dump program.arxmod\n", program_name);
    printf("\n");
}

void print_version(void)
{
    printf("ARX Virtual Machine v1.0\n");
    printf("ARX Programming Language Runtime\n");
    printf("Built with modern C practices\n");
    printf("\n");
}

void print_vm_info(void)
{
    printf("=== ARX Virtual Machine ===\n");
    printf("Version: 1.0\n");
    printf("Architecture: %s\n", 
#ifdef __x86_64__
           "x86_64"
#elif defined(__aarch64__)
           "ARM64"
#elif defined(__arm__)
           "ARM32"
#else
           "Unknown"
#endif
    );
    printf("Debug mode: %s\n", debug_mode ? "enabled" : "disabled");
    printf("\n");
}

bool parse_arguments(int argc, char *argv[], vm_options_t *options)
{
    if (argc < 2) {
        print_usage(argv[0]);
        return false;
    }
    
    // Initialize options
    memset(options, 0, sizeof(vm_options_t));
    
    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-debug") == 0) {
            options->debug_mode = true;
        }
        else if (strcmp(argv[i], "-trace") == 0) {
            options->trace_execution = true;
        }
        else if (strcmp(argv[i], "-dump") == 0) {
            options->dump_state = true;
        }
        else if (strcmp(argv[i], "-step") == 0) {
            options->step_mode = true;
        }
        else if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 < argc) {
                options->output_file = argv[++i];
            } else {
                printf("Error: -o requires an output filename\n");
                return false;
            }
        }
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return false;
        }
        else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            print_version();
            return false;
        }
        else if (argv[i][0] != '-') {
            // Input file
            if (options->input_file == NULL) {
                options->input_file = argv[i];
            } else {
                printf("Error: Multiple input files specified\n");
                return false;
            }
        }
        else {
            printf("Error: Unknown option '%s'\n", argv[i]);
            print_usage(argv[0]);
            return false;
        }
    }
    
    // Validate required arguments
    if (options->input_file == NULL) {
        printf("Error: No input file specified\n");
        print_usage(argv[0]);
        return false;
    }
    
    return true;
}
