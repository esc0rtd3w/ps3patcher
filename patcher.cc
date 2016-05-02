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
  return name+".patched";
}

void patcher::write_patch(int offset, std::ofstream& fout){
  fout.seekp(offset, std::ios::beg);
  fout.write((const char*)&pdata[0], pdata.size());
  status("Patch Data Size: "+std::to_string(pdata.size())+" Bytes");
}

void patcher::do_patch(string name, bool swap){
  string fileout = get_dest_name(name);// Output File
  
  // Strings (Messages)
  string msg_swap = "Byte Swap Enabled [E3 Flasher Style]";
  string msg_ros_patch_1 = "\nApplying ROS Patch 1 of 2";
  string msg_ros_patch_2 = "\nApplying ROS Patch 2 of 2";
  string msg_ros_header_1 = "\nRestoring ROS Header 1 of 2";
  string msg_ros_header_2 = "\nRestoring ROS Header 2 of 2";
  string msg_trvk_patches = "\nApplying TRVK Patches";
  
  // Strings (Patches)
  string p_patch = "patch.bin";
  string p_ros_head = "ros_head.bin";
  string p_nor_rvk = "nor_rvk.bin";
  string p_nand_rvk = "nand_rvk.bin";
  
  std::ifstream freader(name.c_str(), std::ios::binary);
  freader >> std::noskipws;
  std::ofstream fwriter(fileout.c_str(), std::ios::binary);
  
  if(swap) status(msg_swap);
  //status("\nCreating Output File...");
  fwriter << freader.rdbuf(); // copy
  freader.seekg(0, std::ios::end);
  auto pos = freader.tellg();
  freader.close();
  get_ros_pdata(p_patch, swap);
  status("Input Size: "+std::to_string(pos)+" Bytes");
  
  // Check If NOR or NAND
  if(pos!=0x1000000L && pos!=0x10000000L){
    remove(fileout.c_str());
    error("Size Error, Exits");
    return;
  }
  
  // NOR File
  else if(0x1000000L == pos){
    status(msg_ros_patch_1);
    write_patch(0xc0010, fwriter);
    status(msg_ros_patch_2);
    write_patch(0x7c0010, fwriter);
	
    if(chk_flag(0x1)){
      status(msg_trvk_patches);
      get_pdata(p_nor_rvk, swap);
      write_patch(0x40000, fwriter);
    }
	
    if(chk_flag(0x2)){
      status(msg_ros_header_1);
      get_pdata(p_ros_head, swap);
      write_patch(0xc0000, fwriter);
      status(msg_ros_header_2);
      write_patch(0x7c0000, fwriter);
    }
    status("\nAll Patches Applied To NOR");
	
  // NAND File
  }else{
    status(msg_ros_patch_1);
    write_patch(0xc0030, fwriter);
    status(msg_ros_patch_2);
    write_patch(0x7c0020, fwriter);
	
    if(chk_flag(0x1)){
      status(msg_trvk_patches);
      get_pdata(p_nand_rvk, swap);
      write_patch(0x40000, fwriter);
    }
	
    if(chk_flag(0x2)){
      status(msg_ros_header_1);
      get_pdata(p_ros_head, swap);
      write_patch(0xc0020, fwriter);
      status(msg_ros_header_2);
      write_patch(0x7c0010, fwriter);
    }
    status("\nAll Patches Applied To NAND");
  }

  if(chk_flag(0x4)); // autoexit, reserved
  cout << "\nOutput File: " + name + ".patched" + "\n\n";
  return;
}
