#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include "interpreter.h"
#include "ast.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static ErrorList global_errors;

void init_error_list(ErrorList* list) {
    list->head = NULL;
    list->tail = NULL;
    list->count = 0;
}

void add_error(ErrorList* list, int line, const char* msg) {
    ErrorNode* node = (ErrorNode*)malloc(sizeof(ErrorNode));
    node->line = line;
    snprintf(node->message, sizeof(node->message), "%s", msg);
    node->next = NULL;

    if (!list->head) {
        list->head = node;
        list->tail = node;
    } else {
        list->tail->next = node;
        list->tail = node;
    }
    list->count++;
}

void free_error_list(ErrorList* list) {
    ErrorNode* curr = list->head;
    while (curr) {
        ErrorNode* temp = curr;
        curr = curr->next;
        free(temp);
    }
    list->head = NULL;
    list->tail = NULL;
    list->count = 0;
}

void record_lexical_error(int line, const char* msg) {
    add_error(&global_errors, line, msg);
}

void record_syntax_error(int line, const char* msg) {
    add_error(&global_errors, line, msg);
}

typedef struct FrameNode {
    char cmd[64];
    int line;
    double x, y, z;
    double heading;
    double battery;
    bool flying;
    double speed;
    struct FrameNode* next;
} FrameNode;

typedef struct {
    FrameNode* head;
    FrameNode* tail;
    int count;
} FrameList;

static void add_frame(FrameList* frames, const char* cmd, int line, DroneState* s) {
    FrameNode* node = (FrameNode*)malloc(sizeof(FrameNode));
    snprintf(node->cmd, sizeof(node->cmd), "%s", cmd);
    node->line = line;
    node->x = s->x;
    node->y = s->y;
    node->z = s->z;
    node->heading = s->heading;
    node->battery = s->battery;
    node->flying = s->flying;
    node->speed = s->speed;
    node->next = NULL;

    if (!frames->head) {
        frames->head = node;
        frames->tail = node;
    } else {
        frames->tail->next = node;
        frames->tail = node;
    }
    frames->count++;
}

static void exec_node(ASTNode* node, DroneState* s, FrameList* frames, ErrorList* errs) {
    if (!node) return;

    switch (node->type) {
        case AST_PROGRAM:
            for (int i = 0; i < node->data.program.statement_count; i++) {
                exec_node(node->data.program.statements[i], s, frames, errs);
            }
            break;

        case AST_START:
            s->started = true;
            add_frame(frames, "START", node->line, s);
            break;

        case AST_STOP:
            s->started = false;
            if (s->flying) {
                s->flying = false;
                s->z = 0.0;
            }
            add_frame(frames, "STOP", node->line, s);
            break;

        case AST_SET_BATTERY:
            s->battery = node->data.set_battery.battery;
            if (s->battery > 100.0) s->battery = 100.0;
            if (s->battery < 0.0) s->battery = 0.0;
            {
                char cmd_str[64];
                snprintf(cmd_str, sizeof(cmd_str), "SET BATTERY %.0f", s->battery);
                add_frame(frames, cmd_str, node->line, s);
            }
            break;

        case AST_SET_SPEED:
            s->speed = node->data.set_speed.speed;
            if (s->speed <= 0) s->speed = 1.0;
            {
                char cmd_str[64];
                snprintf(cmd_str, sizeof(cmd_str), "SET SPEED %.1f", s->speed);
                add_frame(frames, cmd_str, node->line, s);
            }
            break;

        case AST_TAKEOFF:
            if (!s->started) {
                add_error(errs, node->line, "TAKEOFF issued before START command");
            } else if (s->flying) {
                add_error(errs, node->line, "TAKEOFF command issued while drone is already airborne");
            } else {
                s->flying = true;
                s->z = 2.0; // Standard takeoff altitude
                add_frame(frames, "TAKEOFF", node->line, s);
            }
            break;

        case AST_LAND:
            if (!s->flying) {
                add_error(errs, node->line, "LAND command issued while drone is already on the ground");
            } else {
                s->flying = false;
                s->z = 0.0;
                add_frame(frames, "LAND", node->line, s);
            }
            break;

        case AST_MOVE:
            if (!s->flying) {
                add_error(errs, node->line, "MOVE issued before TAKEOFF");
                break;
            }
            if (s->battery <= 0) {
                add_error(errs, node->line, "Battery depleted (0%) mid-flight during MOVE");
                break;
            }
            {
                double dist = node->data.move.amount * s->speed;
                char cmd_str[64];
                switch (node->data.move.dir) {
                    case DIR_FORWARD: {
                        double rad = s->heading * M_PI / 180.0;
                        s->x += dist * sin(rad);
                        s->y += dist * cos(rad);
                        snprintf(cmd_str, sizeof(cmd_str), "MOVE FORWARD %.1f", node->data.move.amount);
                        break;
                    }
                    case DIR_BACKWARD: {
                        double rad = s->heading * M_PI / 180.0;
                        s->x -= dist * sin(rad);
                        s->y -= dist * cos(rad);
                        snprintf(cmd_str, sizeof(cmd_str), "MOVE BACKWARD %.1f", node->data.move.amount);
                        break;
                    }
                    case DIR_LEFT: {
                        double rad = (s->heading - 90.0) * M_PI / 180.0;
                        s->x += dist * sin(rad);
                        s->y += dist * cos(rad);
                        snprintf(cmd_str, sizeof(cmd_str), "MOVE LEFT %.1f", node->data.move.amount);
                        break;
                    }
                    case DIR_RIGHT: {
                        double rad = (s->heading + 90.0) * M_PI / 180.0;
                        s->x += dist * sin(rad);
                        s->y += dist * cos(rad);
                        snprintf(cmd_str, sizeof(cmd_str), "MOVE RIGHT %.1f", node->data.move.amount);
                        break;
                    }
                    case DIR_UP:
                        s->z += dist;
                        snprintf(cmd_str, sizeof(cmd_str), "MOVE UP %.1f", node->data.move.amount);
                        if (s->z > 30.0) {
                            add_error(errs, node->line, "Altitude exceeds maximum ceiling limit of 30m");
                        }
                        break;
                    case DIR_DOWN:
                        s->z -= dist;
                        if (s->z < 0) s->z = 0;
                        snprintf(cmd_str, sizeof(cmd_str), "MOVE DOWN %.1f", node->data.move.amount);
                        break;
                }

                // Battery drain: dist * 1.2%
                s->battery -= dist * 1.2;
                if (s->battery <= 0) {
                    s->battery = 0;
                    add_error(errs, node->line, "Battery depleted to 0% during flight move");
                }
                add_frame(frames, cmd_str, node->line, s);
            }
            break;

        case AST_TURN:
            if (!s->flying) {
                add_error(errs, node->line, "TURN command issued before TAKEOFF");
                break;
            }
            {
                char cmd_str[64];
                if (node->data.turn.turn_dir == TURN_LEFT) {
                    s->heading -= node->data.turn.angle;
                    snprintf(cmd_str, sizeof(cmd_str), "TURN LEFT %.0f", node->data.turn.angle);
                } else {
                    s->heading += node->data.turn.angle;
                    snprintf(cmd_str, sizeof(cmd_str), "TURN RIGHT %.0f", node->data.turn.angle);
                }
                // Normalize heading to 0-359
                while (s->heading < 0) s->heading += 360.0;
                while (s->heading >= 360.0) s->heading -= 360.0;

                add_frame(frames, cmd_str, node->line, s);
            }
            break;

        case AST_HOVER:
            if (!s->flying) {
                add_error(errs, node->line, "HOVER command issued before TAKEOFF");
                break;
            }
            {
                char cmd_str[64];
                snprintf(cmd_str, sizeof(cmd_str), "HOVER %.1f", node->data.hover.duration);
                s->battery -= node->data.hover.duration * 0.5;
                if (s->battery <= 0) {
                    s->battery = 0;
                    add_error(errs, node->line, "Battery depleted to 0% while hovering");
                }
                add_frame(frames, cmd_str, node->line, s);
            }
            break;

        case AST_REPEAT:
            if (node->data.repeat.count <= 0) {
                add_error(errs, node->line, "REPEAT count must be greater than 0");
                break;
            }
            for (int r = 0; r < node->data.repeat.count; r++) {
                for (int i = 0; i < node->data.repeat.statement_count; i++) {
                    exec_node(node->data.repeat.statements[i], s, frames, errs);
                }
            }
            break;

        case AST_RETURN_HOME:
            if (!s->flying) {
                add_error(errs, node->line, "RETURN_HOME issued before TAKEOFF");
                break;
            }
            {
                double dx = 0.0 - s->x;
                double dy = 0.0 - s->y;
                double dist = sqrt(dx * dx + dy * dy);
                s->x = 0.0;
                s->y = 0.0;
                s->battery -= dist * 1.2;
                if (s->battery < 0) s->battery = 0;
                add_frame(frames, "RETURN_HOME", node->line, s);
            }
            break;
    }
}

void interpret_ast(ASTNode* root, FILE* output_fp) {
    DroneState state = {0.0, 0.0, 0.0, 0.0, 1.0, 100.0, false, false};
    FrameList frames = {NULL, NULL, 0};

    // Include standard frame 0 initial state
    add_frame(&frames, "INIT", 0, &state);

    if (root) {
        exec_node(root, &state, &frames, &global_errors);
    }

    // Print JSON output
    fprintf(output_fp, "{\n");
    fprintf(output_fp, "  \"frames\": [\n");

    FrameNode* curr_f = frames.head;
    while (curr_f) {
        fprintf(output_fp, "    {\n");
        fprintf(output_fp, "      \"cmd\": \"%s\",\n", curr_f->cmd);
        fprintf(output_fp, "      \"line\": %d,\n", curr_f->line);
        fprintf(output_fp, "      \"x\": %.2f,\n", curr_f->x);
        fprintf(output_fp, "      \"y\": %.2f,\n", curr_f->y);
        fprintf(output_fp, "      \"z\": %.2f,\n", curr_f->z);
        fprintf(output_fp, "      \"heading\": %.2f,\n", curr_f->heading);
        fprintf(output_fp, "      \"battery\": %.2f,\n", curr_f->battery);
        fprintf(output_fp, "      \"speed\": %.2f,\n", curr_f->speed);
        fprintf(output_fp, "      \"flying\": %s\n", curr_f->flying ? "true" : "false");
        fprintf(output_fp, "    }%s\n", curr_f->next ? "," : "");
        curr_f = curr_f->next;
    }

    fprintf(output_fp, "  ],\n");

    // Print Errors
    fprintf(output_fp, "  \"errors\": [\n");
    ErrorNode* curr_e = global_errors.head;
    while (curr_e) {
        fprintf(output_fp, "    \"Line %d: %s\"%s\n",
            curr_e->line, curr_e->message, curr_e->next ? "," : "");
        curr_e = curr_e->next;
    }
    fprintf(output_fp, "  ],\n");

    // Print Final State
    fprintf(output_fp, "  \"final_state\": {\n");
    fprintf(output_fp, "    \"x\": %.2f,\n", state.x);
    fprintf(output_fp, "    \"y\": %.2f,\n", state.y);
    fprintf(output_fp, "    \"z\": %.2f,\n", state.z);
    fprintf(output_fp, "    \"heading\": %.2f,\n", state.heading);
    fprintf(output_fp, "    \"battery\": %.2f,\n", state.battery);
    fprintf(output_fp, "    \"flying\": %s\n", state.flying ? "true" : "false");
    fprintf(output_fp, "  }\n");
    fprintf(output_fp, "}\n");

    // Cleanup
    curr_f = frames.head;
    while (curr_f) {
        FrameNode* tmp = curr_f;
        curr_f = curr_f->next;
        free(tmp);
    }
    free_error_list(&global_errors);
}
