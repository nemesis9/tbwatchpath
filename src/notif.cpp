
#include <iostream>
#include <string.h>
#include "tbwatch.h"

//get path of configuration file
std::string 
get_config_path(size_t argc, char ** argv);

//parse config input
bool
parseConfig(std::ifstream& tbconfig, std::vector<std::string>& tdirs);


//entry point
int
main(int argc, char *argv[])
{

    std::vector<std::string> mypaths;

    std::string config_path = get_config_path(argc, argv);
    std::ifstream tbconfig(config_path.c_str());

    if (tbconfig.is_open()) {
     
        bool tbstat = parseConfig(tbconfig, mypaths);
        if (false == tbstat) {
         
            std::cerr << "Error processing configuration file" << std::endl;
            exit(EXIT_FAILURE);
        } 
    }
    else {
     
        std::cerr << "Failed to open configuration file @ " << config_path << std::endl;
        exit(EXIT_FAILURE);
    }

    if (0 == mypaths.size())
    {
        std::cerr << "tbwatch: No watch paths given" << std::endl;
        exit(EXIT_FAILURE);
    }

    //**continue with daemon TbWatch process
    //  initialize the pathwatcher
    TbWatch pathwatch(mypaths);
    //  start the watches
    pathwatch.startWatches();
}


std::string 
get_config_path(size_t argc, char ** argv)
{
    //if arguments provided, see what they are
    //  currently args are not that complicated
    //  so instead of using a library (like getopt)  
    //  we'll do it by hand
    if (argc > 2) {
        if (!strncmp(argv[1], "-c", 2)) {
            return std::string(argv[2]);
        }
    }
    //otherwise try to use default
    return std::string("/etc/tbwatchpaths.cfg");
}


bool
parseConfig(std::ifstream& tbconfig, std::vector<std::string>& mypaths)
{
     //tbconfig is assumed open and good
     size_t lineno = 0;
     bool k = true;
     std::string tbline;
     std::string key, value;

     while (getline(tbconfig, tbline)) {
       
         lineno++;
         if (tbline.size() > 0) {

             k = true;
             key.clear();
             value.clear();

             //ignore comment lines
             if (tbline.at(0) == '#') {
                 continue;
             }

             for (size_t i=0; i<tbline.size(); i++) {                  

                  if (tbline.at(i) == '=') {
                      k = false;
                      continue;
                  }
                  if (true == k) {
                      key.push_back(tbline.at(i));  
                  }
                  else {
                      value.push_back(tbline.at(i)); 
                  }
             }
             //k should be false after parsing the line if it is a key/value pair, since there should have been an = to change the state
             if (true == k) {
                 std::cerr << "Line " << lineno << ":Error, there is no \'=\'-separated key,val pair" << std::endl;
                 exit(EXIT_FAILURE);
             }
             
             if (key == "watch") {
                 mypaths.push_back(value);
             }
             else {
                 //inform user 
                 std::cerr << "Line " << lineno << "<:" << key << ":> is an invalid key, ignored" << std::endl;  
             }
         }             
     }
     return true;
}


