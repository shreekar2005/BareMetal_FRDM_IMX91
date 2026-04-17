import tkinter as tk
from tkinter import messagebox, simpledialog
from tkinter import scrolledtext
import os
import subprocess
import threading
import re

class LittleOSTaskStudio:
    def __init__(self, root):
        self.root = root
        self.root.title("littleOS Studio - Cross-Platform IDE")
        self.root.geometry("950x650")
        
        self.tasks_dir = "tasks"
        os.makedirs(self.tasks_dir, exist_ok=True)
        
        self.current_open_file = None

        self.setup_ui()
        self.setup_syntax_highlighting()
        self.refresh_task_list()

    def setup_ui(self):
        # Left Panel: Controls & Task List
        left_frame = tk.Frame(self.root, width=200, bg="#2e2e2e")
        left_frame.pack(side=tk.LEFT, fill=tk.Y)
        
        tk.Label(left_frame, text="Controls", bg="#2e2e2e", fg="white", font=("Arial", 12, "bold")).pack(pady=10)
        tk.Button(left_frame, text="⚙️ Build OS", command=lambda: self.run_command_async("make")).pack(fill=tk.X, padx=10, pady=5)
        tk.Button(left_frame, text="🧹 Clean OS", command=lambda: self.run_command_async("make clean")).pack(fill=tk.X, padx=10, pady=5)
        
        tk.Frame(left_frame, height=2, bg="#444444").pack(fill=tk.X, padx=10, pady=10)
        
        tk.Button(left_frame, text="➕ New Task", command=self.create_task, bg="#4CAF50", fg="white").pack(fill=tk.X, padx=10, pady=5)
        tk.Button(left_frame, text="🗑️ Delete Task", command=self.delete_task, bg="#d9534f", fg="white").pack(fill=tk.X, padx=10, pady=5)
        
        tk.Label(left_frame, text="Task Registry", bg="#2e2e2e", fg="white", font=("Arial", 10, "bold")).pack(pady=5)
        
        self.task_listbox = tk.Listbox(left_frame, bg="#3e3e3e", fg="white", selectbackground="#5c5c5c", borderwidth=0, highlightthickness=0)
        self.task_listbox.pack(fill=tk.BOTH, expand=True, padx=10, pady=(0, 10))
        self.task_listbox.bind('<<ListboxSelect>>', self.load_task_code)

        # Right-Click Context Menu for the Listbox
        self.context_menu = tk.Menu(self.root, tearoff=0, bg="#3e3e3e", fg="white")
        self.context_menu.add_command(label="🗑️ Delete Task", command=self.delete_task)
        self.task_listbox.bind("<Button-3>", self.show_context_menu)

        # Right Panel: Editor & Console
        right_frame = tk.Frame(self.root, bg="#1e1e1e")
        right_frame.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True)
        
        # Code Editor
        editor_frame = tk.Frame(right_frame, bg="#1e1e1e")
        editor_frame.pack(fill=tk.BOTH, expand=True)
        
        editor_header = tk.Frame(editor_frame, bg="#2e2e2e")
        editor_header.pack(fill=tk.X)
        self.editor_label = tk.Label(editor_header, text="No file opened", font=("Arial", 10, "italic"), bg="#2e2e2e", fg="#aaaaaa")
        self.editor_label.pack(side=tk.LEFT, padx=10, pady=5)
        tk.Button(editor_header, text="💾 Save File", command=self.save_task_code, bg="#007acc", fg="white", borderwidth=0).pack(side=tk.RIGHT, padx=10, pady=5)
        
        self.code_editor = scrolledtext.ScrolledText(editor_frame, wrap=tk.WORD, font=("Consolas", 12), bg="#1e1e1e", fg="#d4d4d4", insertbackground="white", borderwidth=0, highlightthickness=0)
        self.code_editor.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

        # Output Console
        console_frame = tk.Frame(right_frame, height=150, bg="black")
        console_frame.pack(fill=tk.X, side=tk.BOTTOM)
        tk.Label(console_frame, text="Build Output:", anchor="w", bg="#2e2e2e", fg="white").pack(fill=tk.X)
        self.console_output = scrolledtext.ScrolledText(console_frame, height=8, font=("Consolas", 10), bg="black", fg="#4af626", borderwidth=0, highlightthickness=0)
        self.console_output.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

        # Global Keyboard Shortcuts
        self.root.bind('<Control-s>', self.save_task_code)

    def setup_syntax_highlighting(self):
        self.code_editor.tag_config("keyword", foreground="#569cd6", font=("Consolas", 12, "bold"))
        self.code_editor.tag_config("type", foreground="#4ec9b0")
        self.code_editor.tag_config("string", foreground="#ce9178")
        self.code_editor.tag_config("comment", foreground="#6a9955", font=("Consolas", 12, "italic"))
        self.code_editor.tag_config("macro", foreground="#c586c0")
        self.code_editor.tag_config("function", foreground="#dcdcaa")
        
        self.code_editor.bind("<KeyRelease>", self.highlight_syntax)

    def highlight_syntax(self, event=None):
        content = self.code_editor.get("1.0", tk.END)
        
        for tag in ["keyword", "type", "string", "comment", "macro", "function"]:
            self.code_editor.tag_remove(tag, "1.0", tk.END)

        patterns = {
            "comment": r"//.*|/\*[\s\S]*?\*/",
            "string": r'".*?"',
            "macro": r"^\s*#\s*\w+",
            "keyword": r"\b(if|else|for|while|return|switch|case|break|continue|struct|enum|typedef|sizeof)\b",
            "type": r"\b(int|char|float|double|void|bool|volatile|const|uint32_t|uint64_t|uint8_t|uint16_t)\b",
            "function": r"\b([A-Za-z0-9_]+)\s*\("
        }

        for tag, pattern in patterns.items():
            for match in re.finditer(pattern, content, re.MULTILINE):
                start_pos = f"1.0 + {match.start()}c"
                end_pos = f"1.0 + {match.end()}c"
                
                if tag == "function":
                    end_pos = f"1.0 + {match.end()-1}c"
                    
                self.code_editor.tag_add(tag, start_pos, end_pos)

    def show_context_menu(self, event):
        try:
            self.task_listbox.selection_clear(0, tk.END)
            self.task_listbox.selection_set(self.task_listbox.nearest(event.y))
            self.context_menu.tk_popup(event.x_root, event.y_root)
        finally:
            self.context_menu.grab_release()

    def refresh_task_list(self):
        self.task_listbox.delete(0, tk.END)
        for filename in sorted(os.listdir(self.tasks_dir)):
            if filename.endswith(".c"):
                self.task_listbox.insert(tk.END, filename)

    def create_task(self):
        task_name = simpledialog.askstring("New Task", "Enter command name (e.g., sensor_read):")
        if not task_name: return
        
        task_name = task_name.replace(" ", "_").replace(".c", "")
        display_name = simpledialog.askstring("New Task", "Enter Display Name (e.g., Read I2C Temp):")
        if not display_name: display_name = task_name

        filepath = os.path.join(self.tasks_dir, f"{task_name}.c")
        if os.path.exists(filepath):
            messagebox.showwarning("Warning", f"{task_name}.c already exists!")
            return

        c_code = f"""// Task_Name : {display_name}
#include "include/multitasking.h"
#include "include/stdio.h"

void {task_name}_thread(void* arg) {{
    print_dbg("\\r\\n[{task_name.upper()}] Task started!\\r\\n");
    
    // TODO: Add your logic here
    thread_sleep(500); 
    
    print_dbg("\\r\\n[{task_name.upper()}] Task finished!\\r\\n");
}}
"""
        with open(filepath, "w") as f:
            f.write(c_code)
            
        self.refresh_task_list()
        self.log_to_console(f"Created new task template: {filepath}\n")

    def delete_task(self):
        selection = self.task_listbox.curselection()
        if not selection:
            messagebox.showwarning("Warning", "Please select a task to delete.")
            return
            
        filename = self.task_listbox.get(selection[0])
        filepath = os.path.join(self.tasks_dir, filename)

        if messagebox.askyesno("Confirm Delete", f"Are you sure you want to delete '{filename}'?\n\nThis will permanently remove the file."):
            try:
                os.remove(filepath)
                self.log_to_console(f"Deleted task: {filename}\n")
                
                if self.current_open_file == filepath:
                    self.code_editor.delete(1.0, tk.END)
                    self.current_open_file = None
                    self.editor_label.config(text="No file opened")
                    
                self.refresh_task_list()
            except Exception as e:
                messagebox.showerror("Error", f"Failed to delete file:\n{str(e)}")

    def load_task_code(self, event):
        selection = self.task_listbox.curselection()
        if not selection: return
        
        filename = self.task_listbox.get(selection[0])
        self.current_open_file = os.path.join(self.tasks_dir, filename)
        
        with open(self.current_open_file, "r") as f:
            content = f.read()
            
        self.code_editor.delete(1.0, tk.END)
        self.code_editor.insert(tk.END, content)
        self.editor_label.config(text=f"Editing: {filename}")
        
        self.highlight_syntax()

    def save_task_code(self, event=None):
        if not self.current_open_file:
            # We don't want a popup if they just accidentally hit Ctrl+S without a file open
            if event is None: 
                messagebox.showinfo("Info", "No file currently open.")
            return
            
        content = self.code_editor.get(1.0, tk.END)
        with open(self.current_open_file, "w") as f:
            f.write(content.strip() + "\n")
        self.log_to_console(f"Saved: {self.current_open_file}\n")

    def log_to_console(self, text):
        self.console_output.insert(tk.END, text)
        self.console_output.see(tk.END)

    def run_command_async(self, cmd):
        self.log_to_console(f"> {cmd}\n")
        thread = threading.Thread(target=self._execute_cmd, args=(cmd,))
        thread.daemon = True
        thread.start()

    def _execute_cmd(self, cmd):
        process = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        for line in process.stdout:
            self.root.after(0, self.log_to_console, line)
        process.wait()
        self.root.after(0, self.log_to_console, f"Process finished with code {process.returncode}\n\n")

if __name__ == "__main__":
    root = tk.Tk()
    app = LittleOSTaskStudio(root)
    root.mainloop()