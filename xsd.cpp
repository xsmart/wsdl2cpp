#include "xsd.h"

XSD::XSD(string filename)
{
	mPath = "";
	Load(filename);
}

XSD::XSD(xmlDocPtr document)
{
	char path[FILENAME_MAX];
	if (GetCurrentDir(path, sizeof(path)) > 0) {
		mPath = path;
	}
	else {
		mPath = "";
	}
	Load(document);
}

XSD::XSD(xmlNodePtr node)
{
	char path[FILENAME_MAX];
	if (GetCurrentDir(path, sizeof(path)) > 0) {
		mPath = path;
	}
	else {
		mPath = "";
	}
	Load(node);
}

XSD::XSD(xmlDocPtr document, string path)
{
	mPath = path;
	Load(document);
}

XSD::XSD(xmlNodePtr node, string path)
{
	mPath = path;
	Load(node);
}

XSD::~XSD()
{
	for (map<string, XSDElement *>::iterator p = mElements.begin(); p != mElements.end(); p++) {
		delete p->second;
	}
	for (map<string, XSDSimpleType *>::iterator p = mSimpleTypes.begin(); p != mSimpleTypes.end(); p++) {
		delete p->second;
	}
	for (map<string, XSDComplexType *>::iterator p = mComplexTypes.begin(); p != mComplexTypes.end(); p++) {
		delete p->second;
	}
}

string XSD::get_Namespace() const
{
	return mNamespace;
}

vector<string> XSD::get_ElementNames()
{
	vector<string> retval;
	for (auto entry : mElements) {
		retval.push_back(entry.first);
	}
	return retval;
}

XSDElement &XSD::get_Element(string name)
{
	XSDElement *retval = new XSDElement(*mElements[name]);

	return *retval;
}

vector<string> XSD::get_SimpleTypeNames()
{
	vector<string> retval;
	for (auto entry : mSimpleTypes) {
		retval.push_back(entry.first);
	}
	return retval;
}

XSDSimpleType &XSD::get_SimpleType(string name)
{
	XSDSimpleType *retval = new XSDSimpleType(*mSimpleTypes[name]);

	return *retval;
}

vector<string> XSD::get_ComplexTypeNames()
{
	vector<string> retval;
	for (auto entry : mComplexTypes) {
		retval.push_back(entry.first);
	}
	return retval;
}

XSDComplexType &XSD::get_ComplexType(string name)
{
	XSDComplexType *retval = new XSDComplexType(*mComplexTypes[name]);

	return *retval;
}

void XSD::Load(string filename)
{
	mPath = ResolvePath(filename);
	xmlDoc *doc = xmlReadFile(ResolveFilename(filename).c_str(), nullptr, 0);
	if(doc != nullptr) {
		Load(doc);
		xmlFreeDoc(doc);
	}
}

void XSD::Load(xmlDocPtr document)
{
	if (document != nullptr) {
		Load(document->children);
	}
}

void XSD::Load(xmlNodePtr node)
{
	if (node != nullptr && node->ns != nullptr && node->ns->href != nullptr) {
		mNamespace = (char *)node->ns->href;
		if (node->ns->prefix != nullptr) {
			mPrefix = (char *)node->ns->prefix;
		}
		LoadElements(node);
		LoadImports(node);
	}
}

void XSD::LoadElements(xmlNodePtr node)
{
	for (xmlNode *cur_node = node->children; cur_node != nullptr; cur_node = cur_node->next) {
		if (cur_node->type == XML_ELEMENT_NODE && !xmlStrcmp(cur_node->name, (const xmlChar *)"element")) {
			XSDElement *element = new XSDElement(cur_node);
			if (element != nullptr) {
				if (element->get_Name().length() > 0) {
					mElements[element->get_Name()] = element;
				}
				else {
					delete element;
				}
			}
		}
	}
}

void XSD::LoadSimpleTypes(xmlNodePtr node)
{
	for (xmlNode *cur_node = node->children; cur_node != nullptr; cur_node = cur_node->next) {
		if (cur_node->type == XML_ELEMENT_NODE && !xmlStrcmp(cur_node->name, (const xmlChar *)"simpleType")) {
			XSDSimpleType *simpleType = new XSDSimpleType(cur_node);
			if (simpleType != nullptr) {
				if (simpleType->get_Name().length() > 0) {
					mSimpleTypes[simpleType->get_Name()] = simpleType;
				}
				else {
					delete simpleType;
				}
			}
		}
	}
}

void XSD::LoadComplexTypes(xmlNodePtr node)
{
	for (xmlNode *cur_node = node->children; cur_node != nullptr; cur_node = cur_node->next) {
		if (cur_node->type == XML_ELEMENT_NODE && !xmlStrcmp(cur_node->name, (const xmlChar *)"complexType")) {
			XSDComplexType *complexType = new XSDComplexType(cur_node);
			if (complexType != nullptr) {
				if (complexType->get_Name().length() > 0) {
					mComplexTypes[complexType->get_Name()] = complexType;
				}
				else {
					delete complexType;
				}
			}
		}
	}
}

void XSD::LoadImports(xmlNodePtr node)
{
	for(xmlNode *cur_node = node->children; cur_node != nullptr; cur_node = cur_node->next) {
		if(cur_node->type == XML_ELEMENT_NODE && !xmlStrcmp(cur_node->name, (const xmlChar *)"import")) {
			xmlChar *import = xmlGetProp(cur_node, (const xmlChar *)"schemaLocation");
			if(import != nullptr) {
				XSD temp(ResolveFilename((char *)import));
				xmlFree(import);
				for (string element : temp.get_ElementNames()) {
					mElements[element] = new XSDElement(temp.get_Element(element));
				}
				for (string simpleType : temp.get_SimpleTypeNames()) {
					mSimpleTypes[simpleType] = new XSDSimpleType(temp.get_SimpleType(simpleType));
				}
			}
		}
	}
}

string XSD::ResolvePath(string filename)
{
	string retval = filename.substr(0, filename.find_last_of("/\\"));
	if (filename[0] != '/' && filename[0] != '\\' && filename[1] != ':') {
		char path[FILENAME_MAX];
		if (GetCurrentDir(path, sizeof(path)) > 0) {
			string temp = path;
#ifdef _WIN32
			temp += '\\';
#else
			temp += '/';
#endif
			retval = temp + retval;
		}
	}
	return retval;
}

string XSD::ResolveFilename(string filename)
{
	string retval = "";

	if(filename[0] != '/' && filename[0] != '\\' && filename[1] != ':') {
		retval = mPath;
#ifdef _WIN32
		retval += '\\';
#else
		retval += '/';
#endif
	}
#ifdef _WIN32
	else std::replace(filename.begin(), filename.end(), '/', '\\');
#endif
	size_t pos = filename.find_last_of("/\\");
	if (pos == string::npos || (filename[0] == '.' && filename[1] == '.') || filename[0] == '/' || filename[0] == '\\' || filename[1] == ':') retval += filename;
	else retval += filename.substr(pos + 1, filename.length() - pos - 1);

	return retval;
}
