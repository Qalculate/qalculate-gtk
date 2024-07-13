#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>

using namespace std;

int main(int argc, char *argv[]) {
	string sfile = argv[1];
	ifstream cfile(sfile.c_str());
	vector<string> handlers;
	if(cfile.is_open()) {
		string sbuffer;
		while((cfile.rdstate() & std::ifstream::eofbit) == 0) {
			std::getline(cfile, sbuffer);
			size_t i = sbuffer.find("handler=\"on_");
			if(i != string::npos) {
				i += 9;
				size_t i2 = sbuffer.find("\"", i);
				handlers.push_back(sbuffer.substr(i, i2 - i));
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
