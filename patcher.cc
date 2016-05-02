#include <utility>
#include <cstdio> // __LINE__
#include "patcher.h"
#include "pbase.h"
using std::cout;

patcher::patcher(bool dbg):flag(0), debug(dbg){}
patcher::~patcher(){}

void patcher::set_flag(char s){
  flag |= s;
}
char patcher::get_flag(){
  return flag;
}
bool patcher::chk_flag(char s){
  return (flag & s);
}

void patcher::status(string str){
  if(debug) cout << str << std::endl;
}
#define error(str) \
  cout << __FILE__ << ", " << __LINE__ << ": " << str << std::endl;

void patcher::get_pdata(string name, bool swap){
  std::ifstream freader(name.c_str(), std::ios::binary|std::ios::ate);
  freader >> std::noskipws;
  auto pos = freader.tellg();
  std::vector<byte> tmp(pos);

  freader.seekg(0, std::ios::beg);
  freader.read(&tmp[0], pos);
  if(swap) pbase::swap_bytes(tmp); // swap_byte(vector<byte>&, pos_type)

  pdata = std::move(tmp); // c++11 dafahao
  return;
}
void patcher::get_ros_pdata(string name, bool swap){
  get_pdata(name, swap);
}

string patcher::get_dest_name(string name){
  return name+".out";
}

void patcher::write_patch(int offset, std::ofstream& fout){
  fout.seekp(offset, std::ios::beg);
  fout.write((const char*)&pdata[0], pdata.size());
  status("Patch Data Size: "+std::to_string(pdata.size())+" Bytes");
}

void patcher::do_patch(string name, bool swap){
  string fileout = get_dest_name(name);
  std::ifstream freader(name.c_str(), std::ios::binary);
  freader >> std::noskipws;
  std::ofstream fwriter(fileout.c_str(), std::ios::binary);
  if(swap) status("Swap Enabled");
  status("Creating Output File...");
  fwriter << freader.rdbuf(); // copy
  freader.seekg(0, std::ios::end);
  auto pos = freader.tellg();
  freader.close();
  get_ros_pdata("patch.bin", swap);
  status("Input Size: "+std::to_string(pos)+" Bytes");
  if(pos!=0x1000000L && pos!=0x10000000L){
    remove(fileout.c_str());
    error("size error, exits");
    return;
  }
  else if(0x1000000L == pos){ // nor
    status("Applying ROS Patch 1 of 2");
    write_patch(0xc0010, fwriter);
    status("Applying ROS Patch 2 of 2");
    write_patch(0x7c0010, fwriter);
    if(chk_flag(0x1)){
      status("Applying TRVK Patches");
      get_pdata("nor_rvk.bin", swap);
      write_patch(0x40000, fwriter);
    }
    if(chk_flag(0x2)){
      status("Restoring ROS Header 1 of 2");
      get_pdata("ros_head.bin", swap);
      write_patch(0xc0000, fwriter);
      status("Restoring ROS Header 2 of 2");
      write_patch(0x7c0000, fwriter);
    }
    status("All Patches Applied To NOR");
  }else{ // nand
    status("Applying ROS Patch 1 of 2");
    write_patch(0xc0030, fwriter);
    status("Applying ROS Patch 2 of 2");
    write_patch(0x7c0020, fwriter);
    if(chk_flag(0x1)){
      status("Applying TRVK Patches");
      get_pdata("nand_rvk.bin", swap);
      write_patch(0x40000, fwriter);
    }
    if(chk_flag(0x2)){
      status("Restoring ROS Header 1 of 2");
      get_pdata("ros_head.bin", swap);
      write_patch(0xc0020, fwriter);
      status("Restoring ROS Header 2 of 2");
      write_patch(0x7c0010, fwriter);
    }
    status("All Patches Applied To NAND");
  }

  if(chk_flag(0x4)); // autoexit, reserved
  cout << "Done\n";
  return;
}
