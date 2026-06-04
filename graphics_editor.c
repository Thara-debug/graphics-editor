/*
 * 2D Graphics Editor
 * Uses '*' for shapes and '_' for fills/background
 * Supports: Circle, Rectangle, Line, Triangle
 * Operations: Add, Delete, Modify
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ─── Canvas dimensions ─── */
#define ROWS    25
#define COLS    60
#define MAX_OBJ 50

/* ─── Shape types ─── */
#define SHAPE_CIRCLE    1
#define SHAPE_RECT      2
#define SHAPE_LINE      3
#define SHAPE_TRIANGLE  4

/* ─── Canvas ─── */
char canvas[ROWS][COLS];

/* ─── Object descriptor ─── */
typedef struct {
    int  type;
    /* Circle:   x,y = centre, r = radius */
    /* Rect:     x,y = top-left, r = 0, w = width, h = height */
    /* Line:     x,y = start, w,h = end dx,dy (actually x2,y2 stored in w,h) */
    /* Triangle: x,y = vertex-A, w,h = vertex-B, x2,y2 = vertex-C */
    int  x, y, r, w, h;
    int  x2, y2;          /* extra points for triangle / line end */
    int  active;
    char label[16];
} Object;

Object objects[MAX_OBJ];
int    obj_count = 0;

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

void display_canvas(void) {
    /* top border */
    printf("+");
    for (int j = 0; j < COLS; j++) printf("-");
    printf("+\n");

    for (int i = 0; i < ROWS; i++) {
        printf("|");
        for (int j = 0; j < COLS; j++) {
            char c = canvas[i][j];
            /* replace blank interior with underscore for visibility */
            printf("%c", (c == ' ') ? '_' : c);
        }
        printf("|\n");
    }

    /* bottom border */
    printf("+");
    for (int j = 0; j < COLS; j++) printf("-");
    printf("+\n");
}

/* ═══════════════════════════════════════════
   Drawing primitives  (Bresenham / midpoint)
═══════════════════════════════════════════ */

/* --- Line (Bresenham) --- */
void draw_line(int x0, int y0, int x1, int y1) {
    int dx = abs(x1 - x0), dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (1) {
        plot(y0, x0);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
    }
}

/* --- Rectangle (outline) --- */
void draw_rectangle(int x, int y, int w, int h) {
    draw_line(x,     y,     x + w, y    );   /* top    */
    draw_line(x,     y + h, x + w, y + h);   /* bottom */
    draw_line(x,     y,     x,     y + h);   /* left   */
    draw_line(x + w, y,     x + w, y + h);   /* right  */
}

/* --- Circle (midpoint algorithm) --- */
void draw_circle(int cx, int cy, int r) {
    int x = 0, y = r;
    int d = 1 - r;

    while (x <= y) {
        /* 8-way symmetry */
        plot(cy + y, cx + x);  plot(cy - y, cx + x);
        plot(cy + y, cx - x);  plot(cy - y, cx - x);
        plot(cy + x, cx + y);  plot(cy - x, cx + y);
        plot(cy + x, cx - y);  plot(cy - x, cx - y);

        if (d < 0) {
            d += 2 * x + 3;
        } else {
            d += 2 * (x - y) + 5;
            y--;
        }
        x++;
    }
}

/* --- Triangle (three lines) --- */
void draw_triangle(int x0, int y0, int x1, int y1, int x2, int y2) {
    draw_line(x0, y0, x1, y1);
    draw_line(x1, y1, x2, y2);
    draw_line(x2, y2, x0, y0);
}

/* ═══════════════════════════════════════════
   Render all active objects
═══════════════════════════════════════════ */
void render_all(void) {
    clear_canvas();
    for (int i = 0; i < obj_count; i++) {
        Object *o = &objects[i];
        if (!o->active) continue;
        switch (o->type) {
            case SHAPE_CIRCLE:
                draw_circle(o->x, o->y, o->r);
                break;
            case SHAPE_RECT:
                draw_rectangle(o->x, o->y, o->w, o->h);
                break;
            case SHAPE_LINE:
                draw_line(o->x, o->y, o->w, o->h);   /* w,h store x2,y2 */
                break;
            case SHAPE_TRIANGLE:
                draw_triangle(o->x, o->y, o->w, o->h, o->x2, o->y2);
                break;
        }
    }
}

/* ═══════════════════════════════════════════
   Object management
═══════════════════════════════════════════ */
int add_object(Object o) {
    if (obj_count >= MAX_OBJ) {
        printf("Canvas full (max %d objects).\n", MAX_OBJ);
        return -1;
    }
    o.active = 1;
    objects[obj_count] = o;
    return obj_count++;
}

void list_objects(void) {
    printf("\n%-4s %-10s %-6s %-6s %-6s %-6s %-6s %-6s %-6s\n",
           "ID", "Type", "x", "y", "r/x2", "w/y2", "h", "x2", "y2");
    printf("%-4s %-10s %-6s %-6s %-6s %-6s %-6s %-6s %-6s\n",
           "----","----------","------","------","------","------","------","------","------");
    int found = 0;
    for (int i = 0; i < obj_count; i++) {
        Object *o = &objects[i];
        if (!o->active) continue;
        found = 1;
        const char *tname = "?";
        if      (o->type == SHAPE_CIRCLE)   tname = "Circle";
        else if (o->type == SHAPE_RECT)     tname = "Rectangle";
        else if (o->type == SHAPE_LINE)     tname = "Line";
        else if (o->type == SHAPE_TRIANGLE) tname = "Triangle";

        if (o->type == SHAPE_CIRCLE)
            printf("%-4d %-10s %-6d %-6d %-6d\n", i, tname, o->x, o->y, o->r);
        else if (o->type == SHAPE_RECT)
            printf("%-4d %-10s %-6d %-6d %-6s %-6d %-6d\n", i, tname, o->x, o->y, "-", o->w, o->h);
        else if (o->type == SHAPE_LINE)
            printf("%-4d %-10s %-6d %-6d %-6s %-6d %-6d\n", i, tname, o->x, o->y, "-", o->w, o->h);
        else if (o->type == SHAPE_TRIANGLE)
            printf("%-4d %-10s (%d,%d) (%d,%d) (%d,%d)\n",
                   i, tname, o->x,o->y, o->w,o->h, o->x2,o->y2);
    }
    if (!found) printf("  (no objects)\n");
    printf("\n");
}

int delete_object(int id) {
    if (id < 0 || id >= obj_count || !objects[id].active) {
        printf("Invalid ID.\n");
        return 0;
    }
    objects[id].active = 0;
    printf("Object %d deleted.\n", id);
    return 1;
}

int modify_object(int id) {
    if (id < 0 || id >= obj_count || !objects[id].active) {
        printf("Invalid ID.\n");
        return 0;
    }
    Object *o = &objects[id];
    printf("Modifying object %d (%s).\n", id,
           o->type==SHAPE_CIRCLE?"Circle":
           o->type==SHAPE_RECT?"Rectangle":
           o->type==SHAPE_LINE?"Line":"Triangle");

    switch (o->type) {
        case SHAPE_CIRCLE:
            printf("New centre x y and radius: ");
            scanf("%d %d %d", &o->x, &o->y, &o->r);
            break;
        case SHAPE_RECT:
            printf("New top-left x y, width, height: ");
            scanf("%d %d %d %d", &o->x, &o->y, &o->w, &o->h);
            break;
        case SHAPE_LINE:
            printf("New start x1 y1 and end x2 y2: ");
            scanf("%d %d %d %d", &o->x, &o->y, &o->w, &o->h);
            break;
        case SHAPE_TRIANGLE:
            printf("New vertices x1 y1  x2 y2  x3 y3: ");
            scanf("%d %d %d %d %d %d",
                  &o->x, &o->y, &o->w, &o->h, &o->x2, &o->y2);
            break;
    }
    printf("Object %d updated.\n", id);
    return 1;
}

/* ═══════════════════════════════════════════
   Interactive menus
═══════════════════════════════════════════ */
void menu_add(void) {
    printf("\n--- Add Object ---\n");
    printf("1. Circle\n2. Rectangle\n3. Line\n4. Triangle\n0. Back\n");
    printf("Choice: ");
    int ch; scanf("%d", &ch);
    if (ch == 0) return;

    Object o;
    memset(&o, 0, sizeof o);
    o.type = ch;

    switch (ch) {
        case 1: /* Circle */
            printf("Enter centre x y and radius: ");
            scanf("%d %d %d", &o.x, &o.y, &o.r);
            break;
        case 2: /* Rectangle */
            printf("Enter top-left x y, width, height: ");
            scanf("%d %d %d %d", &o.x, &o.y, &o.w, &o.h);
            break;
        case 3: /* Line */
            printf("Enter start x1 y1 and end x2 y2: ");
            scanf("%d %d %d %d", &o.x, &o.y, &o.w, &o.h);
            break;
        case 4: /* Triangle */
            printf("Enter vertices x1 y1  x2 y2  x3 y3: ");
            scanf("%d %d %d %d %d %d",
                  &o.x, &o.y, &o.w, &o.h, &o.x2, &o.y2);
            break;
        default:
            printf("Unknown type.\n"); return;
    }
    int id = add_object(o);
    if (id >= 0) printf("Added as object %d.\n", id);
}

void main_menu(void) {
    int running = 1;
    while (running) {
        render_all();
        display_canvas();
        printf("\n=== 2D Graphics Editor ===\n");
        printf("1. Add object\n");
        printf("2. Delete object\n");
        printf("3. Modify object\n");
        printf("4. List objects\n");
        printf("5. Clear all\n");
        printf("0. Exit\n");
        printf("Choice: ");
        int ch; scanf("%d", &ch);
        switch (ch) {
            case 1: menu_add(); break;
            case 2: {
                list_objects();
                printf("Enter ID to delete: ");
                int id; scanf("%d", &id);
                delete_object(id);
                break;
            }
            case 3: {
                list_objects();
                printf("Enter ID to modify: ");
                int id; scanf("%d", &id);
                modify_object(id);
                break;
            }
            case 4: list_objects(); break;
            case 5:
                for (int i = 0; i < obj_count; i++) objects[i].active = 0;
                printf("Canvas cleared.\n");
                break;
            case 0: running = 0; break;
            default: printf("Invalid choice.\n");
        }
    }
}

/* ═══════════════════════════════════════════
   Demo: pre-load some shapes for testing
═══════════════════════════════════════════ */
void load_demo(void) {
    /* Circle at (30,12) r=8 */
    Object o = {0};
    o.type = SHAPE_CIRCLE; o.x = 30; o.y = 12; o.r = 8;
    add_object(o);

    /* Rectangle (2,2) 14x10 */
    memset(&o, 0, sizeof o);
    o.type = SHAPE_RECT; o.x = 2; o.y = 2; o.w = 14; o.h = 10;
    add_object(o);

    /* Diagonal line */
    memset(&o, 0, sizeof o);
    o.type = SHAPE_LINE; o.x = 45; o.y = 2; o.w = 57; o.h = 14;
    add_object(o);

    /* Triangle */
    memset(&o, 0, sizeof o);
    o.type = SHAPE_TRIANGLE;
    o.x = 20; o.y = 20;
    o.w = 35; o.h = 20;
    o.x2 = 27; o.y2 = 14;
    add_object(o);
}

int main(int argc, char *argv[]) {
    clear_canvas();

    if (argc > 1 && strcmp(argv[1], "--demo") == 0) {
        load_demo();
        render_all();
        display_canvas();
        printf("Demo mode: %d objects loaded.\n", obj_count);
        list_objects();
        return 0;
    }

    //load_demo();          /* pre-populate for convenience */
    main_menu();
    printf("Goodbye!\n");
    return 0;
}
