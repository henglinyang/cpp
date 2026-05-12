#include <libxml/parser.h>
#include <stdio.h>

int main() {
    xmlInitParser();
    LIBXML_TEST_VERSION
    xmlDoc *doc = xmlReadFile("example.xml", NULL, 0);
    if (doc == NULL) {
        fprintf(stderr, "Failed to parse example.xml\n");
        return 1;
    }
    xmlNode *root = xmlDocGetRootElement(doc);
    printf("Root element: %s\n", root->name);
    xmlFreeDoc(doc);
    xmlCleanupParser();
    return 0;
}
