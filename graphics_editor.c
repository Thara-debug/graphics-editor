#define PDC_DLL_BUILD
#define PDC_WIDE
/*
 * 2D Graphics Editor with PDCurses
 * Colored menus, box drawing, arrow key navigation
 * Uses '*' for shapes and '_' for background
 */

#include <curses.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ─── Canvas dimensions ─── */
#define ROWS    20
#define COLS    50
#define MAX_OBJ 50

/* ─── Shape types ─── */
#define SHAPE_CIRCLE    1
#define SHAPE_RECT      2
#define SHAPE_LINE      3
#define SHAPE_TRIANGLE  4

/* ─── Color pairs ─── */
#define CLR_TITLE    1
#define CLR_MENU     2
#define CLR_SELECT   3
#define CLR_CANVAS   4
#define CLR_STATUS   5
#define CLR_SHAPE    6
#define CLR_BORDER   7
#define CLR_TABLE    8

/* ─── Canvas ─── */
char canvas[ROWS][COLS];

/* ─── Object descriptor ─── */
typedef struct {
    int  type;
    int  x, y, r, w, h;
    int  x2, y2;
    int  active;
} Object;

Object objects[MAX_OBJ];
int    obj_count = 0;

/* ─── Windows ─── */
WINDOW *win_title;
WINDOW *win_menu;
WINDOW *win_canvas;
WINDOW *win_status;

/* ═══════════════════════════════════════════
   Canvas helpers
═══════════════════════════════════════════ */
void clear_canvas(void) {
    for (int i = 0; i < ROWS; i++)
        for (int j = 0; j < COLS; j++)
            canvas[i][j] = ' ';
}

void plot(int row, int col) {
    if (row >= 0 && row < ROWS && col >= 0 && col < COLS)
        canvas[row][col] = '*';
}

/* ═══════════════════════════════════════════
   Drawing primitives
═══════════════════════════════════════════ */
void draw_line(int x0, int y0, int x1, int y1) {
    int dx = abs(x1-x0), dy = abs(y1-y0);
    int sx = (x0<x1)?1:-1, sy = (y0<y1)?1:-1;
    int err = dx - dy;
    while(1) {
        plot(y0, x0);
        if(x0==x1 && y0==y1) break;
        int e2 = 2*err;
        if(e2 > -dy){ err -= dy; x0 += sx; }
        if(e2 <  dx){ err += dx; y0 += sy; }
    }
}

void draw_rectangle(int x, int y, int w, int h) {
    draw_line(x,   y,   x+w, y  );
    draw_line(x,   y+h, x+w, y+h);
    draw_line(x,   y,   x,   y+h);
    draw_line(x+w, y,   x+w, y+h);
}

void draw_circle(int cx, int cy, int r) {
    int x=0, y=r, d=1-r;
    while(x<=y){
        plot(cy+y,cx+x); plot(cy-y,cx+x);
        plot(cy+y,cx-x); plot(cy-y,cx-x);
        plot(cy+x,cx+y); plot(cy-x,cx+y);
        plot(cy+x,cx-y); plot(cy-x,cx-y);
        if(d<0) d+=2*x+3;
        else { d+=2*(x-y)+5; y--; }
        x++;
    }
}

void draw_triangle(int x0,int y0,int x1,int y1,int x2,int y2){
    draw_line(x0,y0,x1,y1);
    draw_line(x1,y1,x2,y2);
    draw_line(x2,y2,x0,y0);
}

/* ═══════════════════════════════════════════
   Render all objects
═══════════════════════════════════════════ */
void render_all(void) {
    clear_canvas();
    for(int i=0;i<obj_count;i++){
        Object *o=&objects[i];
        if(!o->active) continue;
        switch(o->type){
            case SHAPE_CIRCLE:   draw_circle(o->x,o->y,o->r); break;
            case SHAPE_RECT:     draw_rectangle(o->x,o->y,o->w,o->h); break;
            case SHAPE_LINE:     draw_line(o->x,o->y,o->w,o->h); break;
            case SHAPE_TRIANGLE: draw_triangle(o->x,o->y,o->w,o->h,o->x2,o->y2); break;
        }
    }
}

/* ═══════════════════════════════════════════
   Display canvas in window
═══════════════════════════════════════════ */
void display_canvas(void) {
    render_all();
    werase(win_canvas);
    wattron(win_canvas, COLOR_PAIR(CLR_BORDER));
    box(win_canvas, 0, 0);
    wattroff(win_canvas, COLOR_PAIR(CLR_BORDER));

    wattron(win_canvas, COLOR_PAIR(CLR_BORDER) | A_BOLD);
    mvwprintw(win_canvas, 0, 2, " CANVAS ");
    wattroff(win_canvas, COLOR_PAIR(CLR_BORDER) | A_BOLD);

    for(int i=0;i<ROWS;i++){
        for(int j=0;j<COLS;j++){
            char c = canvas[i][j];
            if(c=='*'){
                wattron(win_canvas, COLOR_PAIR(CLR_SHAPE) | A_BOLD);
                mvwaddch(win_canvas, i+1, j+1, '*');
                wattroff(win_canvas, COLOR_PAIR(CLR_SHAPE) | A_BOLD);
            } else {
                wattron(win_canvas, COLOR_PAIR(CLR_CANVAS));
                mvwaddch(win_canvas, i+1, j+1, '_');
                wattroff(win_canvas, COLOR_PAIR(CLR_CANVAS));
            }
        }
    }
    wrefresh(win_canvas);
}

/* ═══════════════════════════════════════════
   Status bar
═══════════════════════════════════════════ */
void set_status(const char *msg) {
    werase(win_status);
    wattron(win_status, COLOR_PAIR(CLR_STATUS) | A_BOLD);
    mvwprintw(win_status, 0, 1, " >> %s", msg);
    wattroff(win_status, COLOR_PAIR(CLR_STATUS) | A_BOLD);
    wrefresh(win_status);
}

/* ═══════════════════════════════════════════
   Draw menu with highlight
═══════════════════════════════════════════ */
void draw_menu(int highlight, const char *items[], int n, const char *title) {
    werase(win_menu);
    wattron(win_menu, COLOR_PAIR(CLR_BORDER));
    box(win_menu, 0, 0);
    wattroff(win_menu, COLOR_PAIR(CLR_BORDER));

    wattron(win_menu, COLOR_PAIR(CLR_TITLE) | A_BOLD);
    mvwprintw(win_menu, 0, 2, " %s ", title);
    wattroff(win_menu, COLOR_PAIR(CLR_TITLE) | A_BOLD);

    for(int i=0;i<n;i++){
        if(i==highlight){
            wattron(win_menu, COLOR_PAIR(CLR_SELECT) | A_BOLD);
            mvwprintw(win_menu, i+2, 2, " > %-20s", items[i]);
            wattroff(win_menu, COLOR_PAIR(CLR_SELECT) | A_BOLD);
        } else {
            wattron(win_menu, COLOR_PAIR(CLR_MENU));
            mvwprintw(win_menu, i+2, 2, "   %-20s", items[i]);
            wattroff(win_menu, COLOR_PAIR(CLR_MENU));
        }
    }
    wrefresh(win_menu);
}

/* ═══════════════════════════════════════════
   Get integer input via status bar
═══════════════════════════════════════════ */
int get_int(const char *prompt) {
    werase(win_status);
    wattron(win_status, COLOR_PAIR(CLR_STATUS) | A_BOLD);
    mvwprintw(win_status, 0, 1, " >> %s: ", prompt);
    wattroff(win_status, COLOR_PAIR(CLR_STATUS) | A_BOLD);
    wrefresh(win_status);
    echo();
    curs_set(1);
    int val;
    wscanw(win_status, "%d", &val);
    noecho();
    curs_set(0);
    return val;
}

/* ═══════════════════════════════════════════
   Show objects table
═══════════════════════════════════════════ */
void show_objects_table(void) {
    werase(win_menu);
    wattron(win_menu, COLOR_PAIR(CLR_BORDER));
    box(win_menu, 0, 0);
    wattroff(win_menu, COLOR_PAIR(CLR_BORDER));

    wattron(win_menu, COLOR_PAIR(CLR_TITLE) | A_BOLD);
    mvwprintw(win_menu, 0, 2, " OBJECTS ");
    wattroff(win_menu, COLOR_PAIR(CLR_TITLE) | A_BOLD);

    wattron(win_menu, COLOR_PAIR(CLR_TABLE) | A_BOLD);
    mvwprintw(win_menu, 2, 2, "%-4s %-10s %-4s %-4s %-4s", "ID","Type","X","Y","R/W");
    mvwprintw(win_menu, 3, 2, "%-4s %-10s %-4s %-4s %-4s", "----","----------","----","----","----");
    wattroff(win_menu, COLOR_PAIR(CLR_TABLE) | A_BOLD);

    int row = 4;
    int found = 0;
    for(int i=0;i<obj_count;i++){
        Object *o=&objects[i];
        if(!o->active) continue;
        found = 1;
        const char *tn = o->type==SHAPE_CIRCLE?"Circle":
                         o->type==SHAPE_RECT?"Rectangle":
                         o->type==SHAPE_LINE?"Line":"Triangle";
        wattron(win_menu, COLOR_PAIR(CLR_MENU));
        if(o->type==SHAPE_CIRCLE)
            mvwprintw(win_menu, row, 2, "%-4d %-10s %-4d %-4d r=%-4d", i,tn,o->x,o->y,o->r);
        else if(o->type==SHAPE_RECT)
            mvwprintw(win_menu, row, 2, "%-4d %-10s %-4d %-4d w=%-2d h=%-2d", i,tn,o->x,o->y,o->w,o->h);
        else if(o->type==SHAPE_LINE)
            mvwprintw(win_menu, row, 2, "%-4d %-10s (%d,%d)->(%d,%d)", i,tn,o->x,o->y,o->w,o->h);
        else
            mvwprintw(win_menu, row, 2, "%-4d %-10s 3 pts", i,tn);
        wattroff(win_menu, COLOR_PAIR(CLR_MENU));
        row++;
        if(row > ROWS) break;
    }
    if(!found){
        wattron(win_menu, COLOR_PAIR(CLR_STATUS));
        mvwprintw(win_menu, 4, 2, "  (no objects yet)");
        wattroff(win_menu, COLOR_PAIR(CLR_STATUS));
    }

    wattron(win_menu, COLOR_PAIR(CLR_SELECT));
    mvwprintw(win_menu, ROWS-1, 2, " Press any key to go back ");
    wattroff(win_menu, COLOR_PAIR(CLR_SELECT));
    wrefresh(win_menu);
    wgetch(win_menu);
}

/* ═══════════════════════════════════════════
   Add object menu
═══════════════════════════════════════════ */
void menu_add(void) {
    const char *items[] = {
        "1. Circle",
        "2. Rectangle",
        "3. Line",
        "4. Triangle",
        "0. Back"
    };
    int n = 5, sel = 0;

    while(1){
        draw_menu(sel, items, n, "ADD SHAPE");
        int ch = wgetch(win_menu);
        if(ch == KEY_UP)   sel = (sel-1+n)%n;
        if(ch == KEY_DOWN) sel = (sel+1)%n;
        if(ch == '\n' || ch == KEY_ENTER || ch == 13){
            if(sel==4) return;
            Object o; memset(&o,0,sizeof o);
            o.type = sel+1; o.active = 1;

            switch(o.type){
                case SHAPE_CIRCLE:
                    o.x = get_int("Circle centre X (0-49)");
                    o.y = get_int("Circle centre Y (0-19)");
                    o.r = get_int("Radius");
                    break;
                case SHAPE_RECT:
                    o.x = get_int("Rectangle top-left X");
                    o.y = get_int("Rectangle top-left Y");
                    o.w = get_int("Width");
                    o.h = get_int("Height");
                    break;
                case SHAPE_LINE:
                    o.x = get_int("Line start X1");
                    o.y = get_int("Line start Y1");
                    o.w = get_int("Line end X2");
                    o.h = get_int("Line end Y2");
                    break;
                case SHAPE_TRIANGLE:
                    o.x  = get_int("Vertex A X");
                    o.y  = get_int("Vertex A Y");
                    o.w  = get_int("Vertex B X");
                    o.h  = get_int("Vertex B Y");
                    o.x2 = get_int("Vertex C X");
                    o.y2 = get_int("Vertex C Y");
                    break;
            }
            if(obj_count < MAX_OBJ){
                objects[obj_count++] = o;
                set_status("Shape added successfully!");
            } else {
                set_status("Canvas full!");
            }
            display_canvas();
            return;
        }
    }
}

/* ═══════════════════════════════════════════
   Delete object
═══════════════════════════════════════════ */
void menu_delete(void) {
    show_objects_table();
    int id = get_int("Enter ID to delete");
    if(id>=0 && id<obj_count && objects[id].active){
        objects[id].active = 0;
        set_status("Object deleted!");
        display_canvas();
    } else {
        set_status("Invalid ID!");
    }
}

/* ═══════════════════════════════════════════
   Modify object
═══════════════════════════════════════════ */
void menu_modify(void) {
    show_objects_table();
    int id = get_int("Enter ID to modify");
    if(id<0 || id>=obj_count || !objects[id].active){
        set_status("Invalid ID!"); return;
    }
    Object *o = &objects[id];
    switch(o->type){
        case SHAPE_CIRCLE:
            o->x = get_int("New centre X");
            o->y = get_int("New centre Y");
            o->r = get_int("New radius");
            break;
        case SHAPE_RECT:
            o->x = get_int("New top-left X");
            o->y = get_int("New top-left Y");
            o->w = get_int("New width");
            o->h = get_int("New height");
            break;
        case SHAPE_LINE:
            o->x = get_int("New start X1");
            o->y = get_int("New start Y1");
            o->w = get_int("New end X2");
            o->h = get_int("New end Y2");
            break;
        case SHAPE_TRIANGLE:
            o->x  = get_int("Vertex A X");
            o->y  = get_int("Vertex A Y");
            o->w  = get_int("Vertex B X");
            o->h  = get_int("Vertex B Y");
            o->x2 = get_int("Vertex C X");
            o->y2 = get_int("Vertex C Y");
            break;
    }
    set_status("Object modified!");
    display_canvas();
}

/* ═══════════════════════════════════════════
   Main menu
═══════════════════════════════════════════ */
void main_menu(void) {
    const char *items[] = {
        "1. Add Object",
        "2. Delete Object",
        "3. Modify Object",
        "4. List Objects",
        "5. Clear Canvas",
        "0. Exit"
    };
    int n=6, sel=0;

    while(1){
        draw_menu(sel, items, n, "MAIN MENU");
        display_canvas();
        int ch = wgetch(win_menu);
        if(ch==KEY_UP)   sel=(sel-1+n)%n;
        if(ch==KEY_DOWN) sel=(sel+1)%n;
        if(ch=='\n' || ch==KEY_ENTER || ch==13){
            switch(sel){
                case 0: menu_add(); break;
                case 1: menu_delete(); break;
                case 2: menu_modify(); break;
                case 3: show_objects_table(); break;
                case 4:
                    for(int i=0;i<obj_count;i++) objects[i].active=0;
                    set_status("Canvas cleared!");
                    display_canvas();
                    break;
                case 5: return;
            }
        }
    }
}

/* ═══════════════════════════════════════════
   Setup windows
═══════════════════════════════════════════ */
void setup_windows(void) {
    int maxY, maxX;
    getmaxyx(stdscr, maxY, maxX);

    /* Title bar: full width, 3 rows tall */
    win_title  = newwin(3, maxX, 0, 0);
    /* Menu: left side */
    win_menu   = newwin(maxY-4, 28, 3, 0);
    /* Canvas: right side */
    win_canvas = newwin(ROWS+2, COLS+2, 3, 28);
    /* Status bar: bottom */
    win_status = newwin(1, maxX, maxY-1, 0);

    keypad(win_menu, TRUE);
    keypad(win_canvas, TRUE);
}

void draw_title(void) {
    wattron(win_title, COLOR_PAIR(CLR_TITLE) | A_BOLD);
    werase(win_title);
    box(win_title, 0, 0);
    mvwprintw(win_title, 1, 2, "  2D GRAPHICS EDITOR  |  Use UP/DOWN arrows + ENTER to navigate  |  Shapes: * Background: _");
    wattroff(win_title, COLOR_PAIR(CLR_TITLE) | A_BOLD);
    wrefresh(win_title);
}

/* ═══════════════════════════════════════════
   Main
═══════════════════════════════════════════ */
int main(void) {
    initscr();
    start_color();
    noecho();
    cbreak();
    curs_set(0);

    /* Define color pairs */
    init_pair(CLR_TITLE,  COLOR_BLACK,  COLOR_CYAN);
    init_pair(CLR_MENU,   COLOR_WHITE,  COLOR_BLUE);
    init_pair(CLR_SELECT, COLOR_BLACK,  COLOR_GREEN);
    init_pair(CLR_CANVAS, COLOR_WHITE,  COLOR_BLACK);
    init_pair(CLR_STATUS, COLOR_BLACK,  COLOR_YELLOW);
    init_pair(CLR_SHAPE,  COLOR_GREEN,  COLOR_BLACK);
    init_pair(CLR_BORDER, COLOR_CYAN,   COLOR_BLACK);
    init_pair(CLR_TABLE,  COLOR_YELLOW, COLOR_BLUE);

    setup_windows();
    draw_title();
    clear_canvas();
    display_canvas();
    set_status("Welcome! Use arrow keys to navigate the menu.");

    main_menu();

    endwin();
    printf("Thanks for using 2D Graphics Editor!\n");
    return 0;
}
