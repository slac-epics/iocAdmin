// Tools
#include <ToolsConfig.h>

/// The XML-RPC Server Configuration

/// Reads the config from an XML file
/// that should conform to serverconfig.dtd.
///
class ServerConfig
{
public:
   class Entry
   {
   public:
       void clear()
       {
           key = 0;
           name.assign(0,0);
           path.assign(0,0);
       }
       int key;
       stdString name;
       stdString path;
   };

    stdList<Entry> config;

    /// Read XML file that matches serverconfig.dtd
    bool read(const char *filename);

    /// Finds path for key or returns false.
    bool find(int key, stdString &path); 
    
    void dump();
};


