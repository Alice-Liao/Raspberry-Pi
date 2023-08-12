#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ws2811.h>
#include <unistd.h>

// Define constants for LED configuration
#define TARGET_FREQ WS2811_TARGET_FREQ
#define GPIO_PIN 18
//DMA channel 5 is being selected to handle the data transfer to the LED strip
#define DMA 5
#define LED_COUNT 45

// Initialize the LED string configuration
ws2811_t ledstring = {
    .freq = TARGET_FREQ,
    .dmanum = DMA,
    .channel = {
        [0] = {
            .gpionum = GPIO_PIN,
            .count = LED_COUNT,
            .invert = 0,
            .brightness = 255,
            .strip_type = WS2811_STRIP_RGB,
        },
        [1] = {
            .gpionum = 0,
            .count = 0,
            .invert = 0,
            .brightness = 0,
        },
    },
};

/**
 * A TreeNode structure for creating a binary tree of LEDs.
 * Each node represents an LED with a given index and has pointers to its left and right children.
 */
typedef struct TreeNode {
    int led_index;
    struct TreeNode* left;  // Pointer to the left child node
    struct TreeNode* right; // Pointer to the right child node
} TreeNode;

/**
 * Create a new TreeNode with the given LED index.
 *
 * @param led_index The index of the LED represented by this node.
 * @return A pointer to the newly created TreeNode.
 */
TreeNode* createNode(int led_index) {
    TreeNode* newNode = malloc(sizeof(TreeNode));
    newNode->led_index = led_index;
    newNode->left = NULL;
    newNode->right = NULL;

    return newNode;
}

/**
 * Recursively traverse a binary tree of LEDs and set their colors.
 * LEDs with even indices will be set to color_on, and those with odd indices will be set to color_off.
 *
 * @param root Pointer to the root of the tree to traverse.
 * @param color_on Color for LEDs with even indices.
 * @param color_off Color for LEDs with odd indices.
 */
void traverseTree(TreeNode* root, uint32_t color_on, uint32_t color_off) {
    //uint32_t is likely used to represent the color values of the LEDs. 
    //Each color (red, green, blue) is typically represented by 8 bits, and combining them results in a 24-bit value.
    if (root != NULL) {
        ws2811_led_t color = root->led_index % 2 == 0 ? color_on : color_off;
        ledstring.channel[0].leds[root->led_index] = color;
        traverseTree(root->left, color_on, color_off); // Traverse left subtree
        traverseTree(root->right, color_on, color_off); // Traverse right subtree
    }
}

/**
 * Recursively free all nodes in a binary tree.
 *
 * @param root Pointer to the root of the tree to free.
 */
void freeTree(TreeNode* root) {
    if (root != NULL) {
        freeTree(root->left);  // Free left subtree
        freeTree(root->right); // Free right subtree
        free(root);            // Free the current node
    }
}

int main() {
    ws2811_return_t ret;

    // Initialize the LED string
    if ((ret = ws2811_init(&ledstring)) != WS2811_SUCCESS) {
        fprintf(stderr, "ws2811_init failed: %s\n", ws2811_get_return_t_str(ret));
        return ret;
    }

    // Create a binary tree representation for the LEDs
    TreeNode* root = createNode(0); // Create the root node
    TreeNode* current = root;

    // Build the tree with each LED being represented by a node
    for (int i = 1; i < LED_COUNT; i++) {
        if (i % 2 != 0) {
            current->left = createNode(i);
            current = current->left;
        } else {
            current->right = createNode(i);
            current = current->right;
        }
    }

    int state = 0;
    // Main loop: Toggle LED colors every second
    while (1) {
        state = !state; // Toggle state
        uint32_t evenColor = state == 1 ? 0x00ff00 : 0x000000; // Green or off for even LEDs
        uint32_t oddColor = state == 1 ? 0x000000 : 0xff0000; // Off or red for odd LEDs

        traverseTree(root, evenColor, oddColor);

        ws2811_render(&ledstring);

        usleep(1000000); // Wait for 1 second
    }

    // Cleanup resources
    freeTree(root);
    ws2811_fini(&ledstring);

    return 0;
}