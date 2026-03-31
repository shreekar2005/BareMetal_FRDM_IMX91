import os
import glob

def cleanup_old_files():
    """Aggressively delete old auto-generated files to prevent stale builds."""
    files_to_remove = ["src/autotasks.c", "include/autotasks.h"]
    for f in files_to_remove:
        if os.path.exists(f):
            os.remove(f)

def main():
    cleanup_old_files()

    os.makedirs("include", exist_ok=True)
    os.makedirs("src", exist_ok=True)
    os.makedirs("tasks", exist_ok=True)

    task_files = glob.glob("tasks/*.c")
    tasks = []

    for f in task_files:
        filename = os.path.basename(f)
        cmd_name = filename.replace('.c', '')
        display_name = cmd_name
        
        # Read the first line for the display name
        with open(f, 'r') as file:
            first_line = file.readline().strip()
            if first_line.startswith("// Task_Name :"):
                display_name = first_line.split(":", 1)[1].strip()
        
        tasks.append({
            "cmd": cmd_name,
            "display": display_name[:15], # clamp length for stat table alignment
            "func": f"{cmd_name}_thread", # THIS IS WHERE THE NAMING RULE COMES FROM!
            "id_var": f"{cmd_name}_thread_id"
        })

    with open("include/autotasks.h", "w") as f:
        f.write("// THIS FILE IS AUTO-GENERATED. DO NOT EDIT.\n")
        f.write("#ifndef AUTOTASKS_H\n#define AUTOTASKS_H\n\n")
        f.write("typedef struct {\n")
        f.write("    const char* cmd_string;\n")
        f.write("    const char* display_name;\n")
        f.write("    void (*entrypoint)(void*);\n")
        f.write("    int* id_ptr;\n")
        f.write("} TaskRegistry;\n\n")
        
        f.write(f"extern TaskRegistry autotasks[{max(1, len(tasks))}];\n")
        f.write("extern const int num_autotasks;\n\n")
        f.write("\nvoid init_all_tasks(void);\n\n")
        f.write("#endif\n")

    with open("src/autotasks.c", "w") as f:
        f.write("// THIS FILE IS AUTO-GENERATED. DO NOT EDIT.\n")
        f.write('#include "include/autotasks.h"\n')
        f.write('#include "include/multitasking.h"\n')
        f.write('#include <stddef.h>\n\n')
        
        # Unity Build: Includes actual source files
        for t in tasks:
            f.write(f'#include "../tasks/{t["cmd"]}.c"\n')
        
        f.write("\n")
        for t in tasks:
            f.write(f"int {t['id_var']} = -1;\n")
            
        f.write("\n")
        if tasks:
            f.write(f"TaskRegistry autotasks[{len(tasks)}] = {{\n")
            for t in tasks:
                f.write(f'    {{"{t["cmd"]}", "{t["display"]}", {t["func"]}, &{t["id_var"]}}},\n')
            f.write("};\n")
            f.write(f"const int num_autotasks = {len(tasks)};\n\n")
        else:
            f.write("TaskRegistry autotasks[1] = {0};\n")
            f.write("const int num_autotasks = 0;\n\n")

        f.write("void init_all_tasks(void) {\n")
        for t in tasks:
            f.write(f'    {t["id_var"]} = os_create_thread("{t["display"]}", {t["func"]}, NULL);\n')
        f.write("}\n")

if __name__ == "__main__":
    main()