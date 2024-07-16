#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>

using namespace std;

int main(int argc, char *argv[]) {
	string sfile = argv[1];
	ifstream cfile(sfile.c_str());
	vector<string> handlers;
	string shandler;
	string sprefix = "on_";
	if(cfile.is_open()) {
		string sbuffer;
		while((cfile.rdstate() & std::ifstream::eofbit) == 0) {
			std::getline(cfile, sbuffer);
			for(size_t argi = 2; argi == 2 || argi < argc; argi++) {
				if(argc > argi) sprefix = argv[argi];
				string ssearch = "handler=\"";
				ssearch += sprefix;
				size_t i = sbuffer.find(ssearch);
				if(i != string::npos) {
					i += 9;
					size_t i2 = sbuffer.find("\"", i);
					shandler = sbuffer.substr(i, i2 - i);
					bool b = false;
					for(size_t i = 0; i < handlers.size(); i++) {
						if(handlers[i] == shandler) {b = true; break;}
					}
					if(!b) handlers.push_back(shandler);
				}
			}
		}
		cfile.close();
	}
	cout << "gtk_builder_add_callback_symbols(" << sfile.substr(0, sfile.length() - 3) << "_builder" << ", ";
	for(size_t i = 0; i < handlers.size(); i++) {
		cout << "\"" << handlers[i] << "\", " << "G_CALLBACK(" << handlers[i] << "), ";
	}
	cout << "NULL);" << endl;
	return 0;
}
