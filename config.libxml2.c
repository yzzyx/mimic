#include <stdio.h>
:#include <libxml/parser.h>
#include <libxml/tree.h>

int print_element_names(xmlNode *node)
{
	xmlNode *cur;

	for(cur = node; cur ; cur = cur->next){
		if( cur->type == XML_ELEMENT_NODE ){
			printf("node type element: %s\n", cur->name);
		}else if( cur->type == XML_TEXT_NODE ){
			printf("data: %s\n", cur->name);
		}

		print_element_names(cur->children);
	}

	return 0;
}

int load_config(char *config_file)
{
	xmlDocPtr xmldoc;
	xmlNode *root_node, *node;

	xmldoc = xmlReadFile(config_file, NULL, 0);
	if( xmldoc == NULL ){
		fprintf(stderr, "Failed to parse file '%s'\n", config_file);
		return -1;
	}

	root_node = xmlDocGetRootElement(xmldoc);
	print_element_names(root_node);

	xmlFreeDoc(xmldoc);

	return 0;

}

int main(int argc, char *argv[])
{

	LIBXML_TEST_VERSION

	load_config("mimic.xml");
	xmlCleanupParser();
	return 0;
}
