#include "db/DBTraceAPI.h"

//const string DBTraceAPI::basetable[7]={"Device_Person","Device","Person","Map","BCON","MapMark","Trace"};
const string DBTraceAPI::BASE_TABLE_DEVICE="Device";
const string DBTraceAPI::BASE_TABLE_PERSON="Person";
const string DBTraceAPI::BASE_TABLE_DEVICE_PERSON_RELATE="Device_Person";
const string DBTraceAPI::BASE_TABLE_MAP="Map";
const string DBTraceAPI::BASE_TABLE_BCON="BCON";
const string DBTraceAPI::BASE_TABLE_MAPMARK="MapMark";
const string DBTraceAPI::BASE_TABLE_TRACE="Trace";
using namespace std;

string ptime_to_string(ptime t){
    string time=to_iso_extended_string(t);
    time=time.replace(time.find("T"),1," ");
    return time;
}

//构造函数，保存默认的数据库连接信息
DBTraceAPI::DBTraceAPI(){  
    localhost="localhost";
    user="root";
    passwd="zenos"; 
    database="TraceDB";
    
}
//构造函数，构造时不使用默认参数
DBTraceAPI::DBTraceAPI(string host,string username,string password,string databasename):localhost(host),user(username),passwd(password),database(databasename){}

DBTraceAPI::~DBTraceAPI(){   
}

//初始化，判断mysql服务是否启动
/*
    返回值：
    DB_RET_OK mysql服务以启动成功
    DB_RET_FALL mysql服务安装但未启动
    DB_RET_NULL mysql未安装
*/
extern int checkeprogram(const char* shellstr, const char* str)
{
	int live = 0;
	char line[120];
	FILE* fp = popen(shellstr, "r");
	if(fp == NULL)
	{
		cout << "popen error\n";
		return -1;
	}
	while(!feof(fp)){
		memset(line, 0, sizeof(line));
		fgets(line, sizeof(line), fp);
		if(strstr(line, str) != NULL){
			live++;
			break;
		}
	}
	pclose(fp);
	if(live == 1){
		return 1;
	}
	return 0;
}

int DBTraceAPI::DBInitialize(){
    //int ret = checkeprogram("ps -ef| grep mysql", "mysql");
    int ret = checkeprogram("service mysql status", "Active: active (running)");
	if(ret == 1)
	{
		cout << "mysql server already start"<<endl;
		return DB_RET_OK;
	}else{
        int ret = checkeprogram("service mysql status", "Active: inactive (dead)");
        if(ret==1){
            cout<<"mysql server already showdown"<<endl;
            return DB_RET_FALL;
        }else{
            cout<<"mysql server not install"<<endl;
        }
    }
    return DB_RET_NULL;
}
//建立数据库连接
/*
    返回值：
    DB_RET_OK数据库连接成功
    DB_RET_FALL数据库连接失败
*/
int DBTraceAPI::DBConnect(){
    bool flag_connect=true;
    flag_connect=DB.connect(localhost,user,passwd);
    //判断数据库是否连接成功
    if(flag_connect){
        return DB_RET_OK;
    }else{
        return DB_RET_FALL;
    }
}
//创建数据库以及基础关联表格
/*
    返回值：
    DB_RET_OK数据库创建成功
    DB_RET_CREATE_DB_ERROR数据库创建失败
    DB_RET_CREATE_TB_ERROR数据库关联表创建失败
*/
int DBTraceAPI::DBCreateDB(){
    //判断数据库是否创建成功
    if(DB.createDB(database)==false){
        return DB_RET_CREATE_DB_ERROR;
    }
    DB.useDB(database);
    //判断数据库关联基础表格是否创建成功
    int flag_createTB;
    flag_createTB=DBCreateRelatTB();
    if(flag_createTB==DB_RET_OK){
        return DB_RET_OK;
    }else{
        return DB_RET_CREATE_TB_ERROR;
    }
}
//创建数据库基础关联表格
/*
    返回值：
    DB_RET_OK数据库基础关联表创建成功
    DB_RET_FALL数据库基础关联表创建失败
*/
int DBTraceAPI::DBCreateRelatTB(){
    //通过两个flag判断在创建表的过程中是否出现错误，若出现错误flag_success为false，并且事件回滚到创建表之前
    bool flag_state =true;
    bool flag_success =true;
    DB.useDB(database);
    DB.autoCommitOff();
    //创建设备人员关系表
    flag_state=DB.createTB(BASE_TABLE_DEVICE_PERSON_RELATE,"DeviceID varchar(255) NOT NULL,PersonID int NULL,PersonModule tinyint NULL, PRIMARY KEY (DeviceID)");
    if(flag_state==false){
        flag_success=false;
    }
    //创建设备表
    flag_state=DB.createTB(BASE_TABLE_DEVICE,"DeviceID varchar(255) NOT NULL,TableName char(11) NULL,TraceID int UNSIGNED NULL, PRIMARY KEY (DeviceID)");
    if(flag_state==false){
        flag_success=false;
    }
    //创建人员表
    flag_state=DB.createTB(BASE_TABLE_PERSON,"PersonID int NOT NULL,PersonModule tinyint NULL,TableName char(11) NULL,TraceID int UNSIGNED NULL, PRIMARY KEY (PersonID)");
    if(flag_state==false){
        flag_success=false;
    }
    //创建围栏BCON关系表
    flag_state=DB.createTB(BASE_TABLE_MAP,"MapID int NOT NULL AUTO_INCREMENT,MapMark bigint NULL,BCONID bigint NULL,PRIMARY KEY (MapID)");
    if(flag_state==false){
        flag_success=false;
    }
    //创建BCON表
    flag_state=DB.createTB(BASE_TABLE_BCON,"BCONID bigint NOT NULL,BCONX float NULL,BCONY float NULL,BCONFloor char(4) NULL,PRIMARY KEY (BCONID)");
    if(flag_state==false){
        flag_success=false;
    }
    //创建围栏表
    flag_state=DB.createTB(BASE_TABLE_MAPMARK,"MapMarkID int NOT NULL AUTO_INCREMENT,MapMark bigint NOT NULL,PRIMARY KEY (MapMarkID)");
    if(flag_state==false){
        flag_success=false;
    }
    //创建轨迹表（存储所有具体轨迹表名等信息）
    flag_state=DB.createTB(BASE_TABLE_TRACE,"TableID int UNSIGNED NOT NULL AUTO_INCREMENT,TableName char(11) NULL,YearMonth int NULL,PRIMARY KEY (TableID)");
    if(flag_state==false){
        flag_success=false;
    }
    if(flag_success==true){
        DB.commit();
        //DB.autoCommitOn();
        return DB_RET_OK;
    }else
    {
        DB.rollback();
        //DB.autoCommitOn();
        return DB_RET_FALL;
    }
}
//创建具体轨迹表
/*
    返回值：
    DB_RET_OK具体轨迹表创建成功切将表格信息插入到Trace表中
    DB_RET_CREATE_TB_ERROR具体轨迹表创建失败
    DB_RET_ADD_ERROR具体轨迹表创建成功，但插入到Trace表失败（失败回滚，需重新创表）
*/
int DBTraceAPI::DBCreateTable(const DBTrace&trace){
    
    DB.useDB(database);
    bool flag_create=true;
    bool flag_add=true;
    bool flag_temp=true;
    DB.autoCommitOff();
    //根据轨迹合成表名如Trace202002
    table=BASE_TABLE_TRACE+trace.time.substr(0,4)+trace.time.substr(5,2);
    flag_create=DB.createTB(table,"TraceID int UNSIGNED NOT NULL AUTO_INCREMENT,PersonID bigint NULL,PersonModule tinyint NULL,DeviceID varchar(255) NULL,X float NULL,Y float NULL,Floor char(4) NULL,MapMark bigint NULL,Time datetime NULL,PRIMARY KEY (TraceID)");
    if(flag_create==true){
        int year=atoi(trace.time.substr(0,4).c_str());
        int month=atoi(trace.time.substr(5,2).c_str());
        string yearmonth;
        //将新建的表存入相关月份（一张表对应几个月）
        for(int i=0;i<MemoryMonth;i++){
            if((month+i)>12){
                yearmonth=to_string((year+1)*100+(month+i-12));
            }else{
                yearmonth=to_string((year)*100+(month+i));
            }
            value="(null,'"+table+"', "+yearmonth+")";
            flag_temp=DB.insertItem("Trace",value);
            if(flag_temp==false){
                flag_add=false;
                break;
            }
        }
        if(flag_add==true){
            DB.commit();
            return DB_RET_OK;
        }else{
            DB.rollback();
            return DB_RET_ADD_ERROR;
        }
    }else{
        return DB_RET_CREATE_TB_ERROR;
    }
}
//添加设备以及其初始设备人员关系
/*
    返回值：
    DB_RET_OK成功将设备注册到设备表中，并将其插入人员设备关系表
    DB_RET_FALL在插入两个表中出现错误（回滚）
*/
int DBTraceAPI::DBAddDevice(const DBDeviceData&deviceData){
    DB.useDB(database);
    bool flag_insert_device=true;
    bool flag_insert_relat=true;
    DB.autoCommitOff();
    //将新设备注册到设备表中
    table=BASE_TABLE_DEVICE;
    value="('"+deviceData.DeviceID+"',null,null)";
    flag_insert_device=DB.insertItem(table,value);
    //将新设备和对应人员关系更新到关系表中
    table=BASE_TABLE_DEVICE_PERSON_RELATE;
    value="('"+deviceData.DeviceID+"', "+to_string(deviceData.PersonID)+", "+to_string(deviceData.PersonModule)+")";
    flag_insert_relat=DB.insertItem(table,value);
    if(flag_insert_device&&flag_insert_relat){
        DB.commit();
        return DB_RET_OK;
    }else{
        DB.rollback();
        return DB_RET_FALL;
    }
}
//设备批量添加设备
/*
    返回值：
    DB_RET_OK成功将设备注册到设备表中，并将其插入人员设备关系表
    DB_RET_FALL在插入两个表中出现错误（回滚）
*/
int DBTraceAPI::DBAddDevice(const vector<DBDeviceData>&deviceData){
    DB.useDB(database);
    //获得批量输入设备行数
    //int row=deviceData.size();
    //if(row==0){
    if(deviceData.empty()){
        return DB_RET_NULL;
    }
    DB.autoCommitOff();
    bool flag_first=true;
    string value1;
    string value2;
    for(auto &v:deviceData){
        if(flag_first){
            value1="('"+v.DeviceID+"',null,null)";
            value2="('"+v.DeviceID+"', "+to_string(v.PersonID)+", "+to_string(v.PersonModule)+")";
            flag_first=false;
        }else{
            value1=value1+",('"+v.DeviceID+"',null,null)";
            value2=value2+",('"+v.DeviceID+"', "+to_string(v.PersonID)+", "+to_string(v.PersonModule)+")";
        }
    }
    //将设备添加到设备表中
    table=BASE_TABLE_DEVICE;
    value=value1;
    if(DB.insertItem(table,value)==false){
        DB.rollback();
        return DB_RET_FALL;
    }
    //将设备与人员关系添加到关系表中
    table=BASE_TABLE_DEVICE_PERSON_RELATE;
    value=value2;
    if(DB.insertItem(table,value)==false){
        DB.rollback();
        return DB_RET_FALL;
    }

    DB.commit();
    return DB_RET_OK;
}
//添加人员
/*
    返回值：
    DB_RET_OK成功将人员注册到人员表中
    DB_RET_FALL在插入人员表中出现错误
*/
int DBTraceAPI::DBAddPerson(const DBDeviceData&deviceData){
    DB.useDB(database);
    bool flag_insert_person=true;
    //将新人员注册到人员表中
    table=BASE_TABLE_PERSON;
    value="("+to_string(deviceData.PersonID)+", "+to_string(deviceData.PersonModule)+",null,null)";
    flag_insert_person=DB.insertItem(table,value);
    if(flag_insert_person){
        return DB_RET_OK;
    }else{
        return DB_RET_FALL;
    }
}
//设备批量添加人员
/*
    返回值：
    DB_RET_OK成功将人员注册到人员表中
    DB_RET_FALL在插入人员表中出现错误（回滚）
*/
int DBTraceAPI::DBAddPerson(const vector<DBDeviceData>&deviceData){
    DB.useDB(database);
    //获得批量输入设备行数
    //int row=deviceData.size();
    if(deviceData.empty()){
        return DB_RET_NULL;
    }
    DB.autoCommitOff();
    bool flag_first=true;
    for(auto &v:deviceData){
        //将设备添加到设备表中
        if(flag_first){
            value="("+to_string(v.PersonID)+", "+to_string(v.PersonModule)+",null,null)";
            flag_first=false;
        }else{
            value=value+",("+to_string(v.PersonID)+", "+to_string(v.PersonModule)+",null,null)";
        }   
    }
    table=BASE_TABLE_PERSON;
    if(DB.insertItem(table,value)==false){
        DB.rollback();
        return DB_RET_FALL;
    }
    DB.commit();
    return DB_RET_OK;
}
//更新设备和人员关系
/*
    返回值：
    DB_RET_OK成功更新人员关系表
    DB_RET_FALL更新人员关系表失败
*/
int DBTraceAPI::DBUpdateDeviceRelat(const DBDeviceData&deviceData){
    DB.useDB(database);
    bool flag_updata=true;
    table=BASE_TABLE_DEVICE_PERSON_RELATE;
    value="PersonID="+to_string(deviceData.PersonID)+", PersonModule="+to_string(deviceData.PersonModule);
    limits="DeviceID='"+deviceData.DeviceID+"'";
    flag_updata=DB.updateItem(table,value,limits);
    if(flag_updata){
        return DB_RET_OK;
    }else{
        return DB_RET_FALL;
    }
}
//批量更新设备和人员关系
/*
    返回值：
    DB_RET_OK成功更新人员关系表
    DB_RET_FALL更新人员关系表失败（回滚）
*/
int DBTraceAPI::DBUpdateDeviceRelat(const vector<DBDeviceData>&deviceData){
    DB.useDB(database);
    bool flag_updata=true;
    //int row=deviceData.size();
    if(deviceData.empty()){
        return DB_RET_NULL;
    }
    DB.autoCommitOff();
    table=BASE_TABLE_DEVICE_PERSON_RELATE;
    //for(int i=0;i<row;i++){
    for(auto &v:deviceData){
        value="PersonID="+to_string(v.PersonID)+", PersonModule="+to_string(v.PersonModule);
        limits="DeviceID='"+v.DeviceID+"'";
        if(DB.updateItem(table,value,limits)==false){
            flag_updata=false;
            break;
        }
    }
    if(flag_updata){
        DB.commit();
        return DB_RET_OK;
    }else{
        DB.rollback();
        return DB_RET_FALL;
    }
}
//添加单条围栏与BCON关联信息
/*
    返回值：
    DB_RET_OK成功添加围栏和BCON关联
    DB_RET_FALL添加围栏和BCON关联失败
*/
int DBTraceAPI::DBAddMap(const Map&map){
    DB.useDB(database);
    bool flag_add=true;
    table=BASE_TABLE_MAP;
    value="(null,"+to_string(map.MapMark)+", "+to_string(map.BCONID)+")";
    flag_add=DB.insertItem(table,value);
    if(flag_add){
        return DB_RET_OK;
    }else{
        return DB_RET_FALL;
    }
}
//添加多条围栏与BCON关联信息
/*
    返回值：
    DB_RET_OK成功添加围栏和BCON关联
    DB_RET_FALL添加围栏和BCON关联失败（回滚）
*/
int DBTraceAPI::DBAddMap(const vector<Map>&map){
    DB.useDB(database);
    //读取需要添加的围栏信息
    //int row=map.size();
    if(map.empty()){
        return DB_RET_NULL;
    }
    DB.autoCommitOff();
    table=BASE_TABLE_MAP;
    bool flag_first=true;
    for(auto &v:map){
        if(flag_first){
            value="(null,"+to_string(v.MapMark)+", "+to_string(v.BCONID)+")";
            flag_first=false;
        }else{
            value=value+",(null,"+to_string(v.MapMark)+", "+to_string(v.BCONID)+")";
        }   
    }
    if(DB.insertItem(table,value)==false){
        DB.rollback();
        return DB_RET_FALL;
    }
    DB.commit();
    return DB_RET_OK;
}
//添加一条BCON信息
/*
    返回值：
    DB_RET_OK成功添加BCON
    DB_RET_FALL添加BCON失败
*/
int DBTraceAPI::DBAddBCON(const BCON&bcon){
    DB.useDB(database);
    bool flag_add=true;
    table=BASE_TABLE_BCON;
    value="("+to_string(bcon.BCONID)+", "+to_string(bcon.BCONX)+", "+to_string(bcon.BCONY)+", '"+bcon.Floor+"')";
    flag_add=DB.insertItem(table,value);
    if(flag_add){
        return DB_RET_OK;
    }else{
        return DB_RET_FALL;
    }
}
//添加多条BCON信息
/*
    返回值：
    DB_RET_OK成功添加BCON
    DB_RET_FALL添加BCON失败（回滚）
*/
int DBTraceAPI::DBAddBCON(const vector<BCON>&bcon){
    DB.useDB(database);
    //读取需要添加的BCON信息
    //int row=bcon.size();
    if(bcon.empty()){
        return DB_RET_NULL;
    }
    DB.autoCommitOff();
    table=BASE_TABLE_BCON;
    bool flag_first=true;
    for(auto &v:bcon){
        if(flag_first){
            value="("+to_string(v.BCONID)+", "+to_string(v.BCONX)+", "+to_string(v.BCONY)+", '"+v.Floor+"')";
            flag_first=false;
        }else{
            value=value+",("+to_string(v.BCONID)+", "+to_string(v.BCONX)+", "+to_string(v.BCONY)+", '"+v.Floor+"')";
        }
    }
    if(DB.insertItem(table,value)==false){
        DB.rollback();
        return DB_RET_FALL;
    }
    DB.commit();
    return DB_RET_OK;
}
//添加单条围栏信息
/*
    返回值：
    DB_RET_OK成功添加围栏
    DB_RET_FALL添加围栏失败
*/
int DBTraceAPI::DBAddMapMark(int mapMark){
    DB.useDB(database);
    bool flag_add=true;
    table=BASE_TABLE_MAPMARK;
    value="(null,"+to_string(mapMark)+")";
    flag_add=DB.insertItem(table,value);
    if(flag_add){
        return DB_RET_OK;
    }else{
        return DB_RET_FALL;
    }
}
//添加多条围栏信息
/*
    返回值：
    DB_RET_OK成功添加围栏
    DB_RET_FALL添加围栏失败（回滚）
*/
int DBTraceAPI::DBAddMapMark(const vector<int>&mapMark){
    DB.useDB(database);
    //读取需要添加的BCON信息
    //int row=mapMark.size();
    if(mapMark.empty()){
        return DB_RET_NULL;
    }
    DB.autoCommitOff();
    table=BASE_TABLE_MAPMARK;
    bool flag_first=true;
    for(auto &v:mapMark){
        if(flag_first){
            value="(null,"+to_string(v)+")";
            flag_first=false;
        }else{
            value=value+",(null,"+to_string(v)+")";
        }
    }
    if(DB.insertItem(table,value)==false){
        DB.rollback();
        return DB_RET_FALL;
    }
    DB.commit();
    return DB_RET_OK;
}
//更新设备最新轨迹的位置到设备表
/*
    返回值：
    DB_RET_OK成功更新设备最新轨迹的位置到设备表
    DB_RET_FALL更新设备最新轨迹的位置到设备表失败
*/
int DBTraceAPI::DBUpdateDevice(string DeviceID,string TableName,int TraceID){
    DB.useDB(database);
    bool flag_updata=true;
    table=BASE_TABLE_DEVICE;
    value="TableName='"+TableName+"', TraceID="+to_string(TraceID);
    limits="DeviceID='"+DeviceID+"'";
    flag_updata=DB.updateItem(table,value,limits);
    if(flag_updata){
        return DB_RET_OK;
    }else{
        return DB_RET_FALL;
    }
}
//更新设备最新轨迹的位置到人员表
/*
    返回值：
    DB_RET_OK成功更新人员最新轨迹的位置到设备表
    DB_RET_FALL更新人员最新轨迹的位置到设备表失败
*/
int DBTraceAPI::DBUpdatePerson(int PersonID,int PersonModule,string TableName,int TraceID){
    DB.useDB(database);
    bool flag_updata=true;
    table=BASE_TABLE_PERSON;
    value="TableName='"+TableName+"', TraceID="+to_string(TraceID);
    limits="PersonID="+to_string(PersonID)+" AND PersonModule="+to_string(PersonModule);
    flag_updata=DB.updateItem(table,value,limits);
    if(flag_updata){
        return DB_RET_OK;
    }else{
        return DB_RET_FALL;
    }
}
//添加单条轨迹信息
/*
    返回值：
    DB_RET_OK成功添加单条轨迹
    DB_RET_FALL添加单条轨迹插入轨迹表失败或更新设备表/人员表最新轨迹失败（回滚）
    DB_RET_CREATE_TB_ERROR新建表格失败
    DB_RET_ERORR Trace表中存在错误（一个月份信息对应多张表）
*/
int DBTraceAPI::DBAddTrace(const DBTrace&trace){
    DB.useDB(database);
    table=BASE_TABLE_TRACE;
    value="TableName";
    //保存轨迹的年月信息以搜索相关表
    limits="YearMonth="+trace.time.substr(0,4)+trace.time.substr(5,2);
    string_table ret=DB.selectItem(table,value,limits);
    //判断是否有存储轨迹的相关表，如果没有这创建新表并且重新调用添加函数，有则直接进行存储
    if(ret.size()==0){
        if(DBCreateTable(trace)==DB_RET_OK){
            table=BASE_TABLE_TRACE+trace.time.substr(0,4)+trace.time.substr(5,2);
        }else{
            return DB_RET_CREATE_TB_ERROR;
        }
    }else if(ret.size()==1){
        table=ret[0][0];    
    }else{
        return DB_RET_ERORR;
    }
    bool flag_insert=true;
    int TraceID;
    DB.autoCommitOff();
    string tableName=table;
    value="(null,"+to_string(trace.PersonID)+", "+to_string(trace.PersonModule)+", '"+trace.DeviceID+"', "+to_string(trace.X)+", "+to_string(trace.Y)+", '"+trace.Floor+"', "+to_string(trace.MapMark)+", '"+trace.time+"')";
    if(DB.insertItem(table,value)==false){
        flag_insert=false;
    }else{
        value="TraceID";
        limits="PersonID="+to_string(trace.PersonID)+" AND PersonModule="+to_string(trace.PersonModule)+" AND DeviceID='"+trace.DeviceID+"' AND X="+to_string(trace.X)+" AND Y="+to_string(trace.Y)+" AND Floor='"+trace.Floor+"' AND MapMark="+to_string(trace.MapMark)+" AND Time='"+trace.time+"'";
        string_table ret=DB.selectItem(table,value,limits);
        TraceID=atoi(ret[0][0].c_str());
        if(DBUpdateDevice(trace.DeviceID,tableName,TraceID)!=DB_RET_OK){
            flag_insert=false;
        }
        if(DBUpdatePerson(trace.PersonID,trace.PersonModule,tableName,TraceID)!=DB_RET_OK){
            flag_insert=false;
        }
    }
    if(flag_insert){
        DB.commit();
        return DB_RET_OK;
    }else{
        DB.rollback();
        return DB_RET_FALL;
    }
}
//添加多条轨迹信息
/*
    返回值：
    DB_RET_OK成功添加轨迹
    DB_RET_FALL添加轨迹失败（回滚）
    DB_RET_NULL需要插入的轨迹为空
*/
int DBTraceAPI::DBAddTrace(const vector<DBTrace>&trace){
    DB.useDB(database);
    //查看所需存储的轨迹条数
    //int row=trace.size();
    if(trace.empty()){
        return DB_RET_NULL;
    }
    //bool flag_add=true;
    vector<DBTrace> temptrace_p;
    vector<DBTrace> temptrace_d;
    for(auto v:trace){
        if(temptrace_p.empty()){
            temptrace_p.push_back(v);
        }else{
            bool flag_t=true;
            for(unsigned int i=0;i<temptrace_p.size();i++){
                if(temptrace_p[i].PersonID==v.PersonID&&temptrace_p[i].PersonModule==v.PersonModule){
                    ptime tm1=time_from_string(v.time);
                    ptime tm2=time_from_string(temptrace_p[i].time);
                    if(tm1>=tm2){
                        temptrace_p[i].DeviceID=v.DeviceID;
                        temptrace_p[i].X=v.X;
                        temptrace_p[i].Y=v.Y;
                        temptrace_p[i].Floor=v.Floor;
                        temptrace_p[i].MapMark=v.MapMark;
                        temptrace_p[i].time=v.time;
                    }
                    flag_t=false;
                }
            }
            if(flag_t==true){
                temptrace_p.push_back(v);
            }
        }
        if(temptrace_d.empty()){
            temptrace_d.push_back(v);
        }else{
            bool flag_t=true;
            for(unsigned int i=0;i<temptrace_d.size();i++){
                if(temptrace_d[i].DeviceID==v.DeviceID){
                    ptime tm1=time_from_string(v.time);
                    ptime tm2=time_from_string(temptrace_d[i].time);
                    if(tm1>=tm2){
                        temptrace_d[i].PersonID=v.PersonID;
                        temptrace_d[i].PersonModule=v.PersonModule;
                        temptrace_d[i].X=v.X;
                        temptrace_d[i].Y=v.Y;
                        temptrace_d[i].Floor=v.Floor;
                        temptrace_d[i].MapMark=v.MapMark;
                        temptrace_d[i].time=v.time;
                    }
                    flag_t=false;
                }
            }
            if(flag_t==true){
                temptrace_d.push_back(v);
            }
        }
    }
    

    bool flag_frist=true;
    DB.autoCommitOff();
    string yearmonth="000000";
    for(auto &v:trace){
        if(yearmonth!=(v.time.substr(0,4)+v.time.substr(5,2))){
            if(flag_frist!=true){
                limits="(TraceID,PersonID,PersonModule,DeviceID,X,Y,Floor,MapMark,Time)";
                if(DB.insertItem(table,value,limits)==false){
                    DB.rollback();
                    return DB_RET_ADD_ERROR;
                }
                string_table ret=DB.selectItem(table,"distinct PersonID");
            }
            table=BASE_TABLE_TRACE;
            value="TableName";
            yearmonth=v.time.substr(0,4)+v.time.substr(5,2);
            limits="YearMonth="+v.time.substr(0,4)+v.time.substr(5,2);
            string_table ret=DB.selectItem(table,value,limits);
            if(ret.size()==0){
                if(DBCreateTable(v)==DB_RET_OK){
                    table=BASE_TABLE_TRACE+v.time.substr(0,4)+v.time.substr(5,2);
                    flag_frist=true;
                }else{
                    DB.rollback();
                    return DB_RET_CREATE_TB_ERROR;
                }
            }else if(ret.size()==1){
                table=ret[0][0];
                flag_frist=true;
            }else{
                DB.rollback();
                return DB_RET_ERORR;
            }
        }
        if(flag_frist){
            value="(null,"+to_string(v.PersonID)+", "+to_string(v.PersonModule)+", '"+v.DeviceID+"', "+to_string(v.X)+", "+to_string(v.Y)+", '"+v.Floor+"', "+to_string(v.MapMark)+", '"+v.time+"')";
            flag_frist=false;
        }else{
            value=value+",(null,"+to_string(v.PersonID)+", "+to_string(v.PersonModule)+", '"+v.DeviceID+"', "+to_string(v.X)+", "+to_string(v.Y)+", '"+v.Floor+"', "+to_string(v.MapMark)+", '"+v.time+"')";
        }
    }
    limits="(TraceID,PersonID,PersonModule,DeviceID,X,Y,Floor,MapMark,Time)";
    if(DB.insertItem(table,value,limits)==false){
        DB.rollback();
        return DB_RET_ADD_ERROR;
    }else{

        for(auto p:temptrace_p){
            table=BASE_TABLE_TRACE;
            value="TableName";
            limits="YearMonth="+p.time.substr(0,4)+p.time.substr(5,2);
            string_table ret1=DB.selectItem(table,value,limits);
            string tableName=ret1[0][0];

            table=tableName;
            value="TraceID";
            limits="PersonID="+to_string(p.PersonID)+" AND PersonModule="+to_string(p.PersonModule)+" AND DeviceID='"+p.DeviceID+"' AND X="+to_string(p.X)+" AND Y="+to_string(p.Y)+" AND Floor='"+p.Floor+"' AND MapMark="+to_string(p.MapMark)+" AND Time='"+p.time+"'";
            string_table ret2=DB.selectItem(table,value,limits);
            int TraceID;
            TraceID=atoi(ret2[0][0].c_str());
            if(DBUpdatePerson(p.PersonID,p.PersonModule,tableName,TraceID)!=DB_RET_OK){
                DB.rollback();
                return DB_RET_ADD_ERROR;
            }
        }
        for(auto d:temptrace_d){
            table=BASE_TABLE_TRACE;
            value="TableName";
            limits="YearMonth="+d.time.substr(0,4)+d.time.substr(5,2);
            string_table ret1=DB.selectItem(table,value,limits);
            string tableName=ret1[0][0];

            table=tableName;
            value="TraceID";
            limits="PersonID="+to_string(d.PersonID)+" AND PersonModule="+to_string(d.PersonModule)+" AND DeviceID='"+d.DeviceID+"' AND X="+to_string(d.X)+" AND Y="+to_string(d.Y)+" AND Floor='"+d.Floor+"' AND MapMark="+to_string(d.MapMark)+" AND Time='"+d.time+"'";
            string_table ret2=DB.selectItem(table,value,limits);
            int TraceID;
            TraceID=atoi(ret2[0][0].c_str());
            if(DBUpdateDevice(d.DeviceID,tableName,TraceID)!=DB_RET_OK){
                DB.rollback();
                return DB_RET_ADD_ERROR;
            }
        }
        DB.commit();
        return DB_RET_OK;
    }
}
//搜索某一设备最近一条的轨迹信息
/*
    PTrace用来回传轨迹信息将会包含（TraceID，PersonID,PersonModule,DeviceID,X,Y,Floor,MapMark,Time)
    返回值：
    DB_RET_OK成功查询轨迹
    DB_RET_DEVICE_ERROR设备无最近轨迹记录（有能轨迹已被删除但未在设备表中更新）
    DB_RET_NULL设备无最近轨迹记录
*/
int DBTraceAPI::DBSearchDevice(string DeviceID,DBTrace&pTrace){
    DB.useDB(database);
    table=BASE_TABLE_DEVICE;
    value="TableName,TraceID";
    limits="DeviceID='"+DeviceID+"' AND TraceID IS NOT NULL";
    //搜索对应设备在设备表中记录的最近一条记录
    string_table ret=DB.selectItem(table,value,limits);
    if(ret.size()==0){
        return DB_RET_DEVICE_ERROR;
    }else if(ret.size()==1){
        //根据最近一条的轨迹信息来搜索轨迹具体信息
        table=ret[0][0];
        value="*";
        limits="TraceID="+ret[0][1];
        string_table temp=DB.selectItem(table,value,limits);
        if(temp.size()==0){
            return DB_RET_NULL;
        }
        pTrace=pTrace.readTrace(temp);
    }
    return DB_RET_OK;
}
//搜索某几个设备最近一条的轨迹信息
/*
    Trace用来回传轨迹信息将会包含（TraceID，PersonID,PersonModule,DeviceID,X,Y,Floor,MapMark,Time)
    返回值：
    DB_RET_OK成功查询轨迹
    DB_RET_NULL需查询设备无最近轨迹记录
*/
int DBTraceAPI::DBSearchDevice(const vector<string>&DeviceID,vector<DBTrace>&Trace){
    DB.useDB(database);
    for(auto &v:DeviceID){
        DBTrace tempTrace;
        if(DBSearchDevice(v,tempTrace)==DB_RET_OK){
            Trace.push_back(tempTrace);
        }
    }

    if(Trace.size()==0){
        return DB_RET_NULL;
    }else{
        return DB_RET_OK;
    }
}
//搜索所有设备最近一条的轨迹信息
/*
    Traces用来回传轨迹信息将会包含（TraceID，PersonID,PersonModule,DeviceID,X,Y,Floor,MapMark,Time)
    返回值：
    DB_RET_OK成功查询轨迹
    DB_RET_NULL需查询设备无最近轨迹记录
*/
int DBTraceAPI::DBSearchDevice(vector<DBTrace>&Traces){
    DB.useDB(database);
    string_table tablename;
    //读取所有设备
    table=BASE_TABLE_DEVICE;
    value="TableName,TraceID";
    limits="TraceID IS NOT NULL";
    tablename=DB.selectItem(table,value,limits);
    //int rows=tablename.size();
    if(tablename.empty()){
        return DB_RET_DEVICE_ERROR;
    }
    value="*";
    for(auto &v:tablename){
        table=v[0];
        limits="TraceID="+v[1];
        string_table temp=DB.selectItem(table,value,limits);
        DBTrace trace;
        trace=trace.readTrace(temp);
        Traces.push_back(trace);
    }

    if(Traces.size()==0){
        return DB_RET_NULL;
    }else{
        return DB_RET_OK;
    }
}
//搜索某一人最近一条的轨迹信息
/*
    PTrace用来回传轨迹信息将会包含（TraceID，PersonID,PersonModule,DeviceID,X,Y,Floor,MapMark,Time)
    返回值：
    DB_RET_OK成功查询轨迹
    DB_RET_Person_ERROR设备无最近轨迹记录
    DB_RET_NULL设备无最近轨迹记录（有能轨迹已被删除但未在人员表中更新）
*/
int DBTraceAPI::DBSearchPerson(int PersonID,int PersonModule,DBTrace&pTrace){
    DB.useDB(database);
    table=BASE_TABLE_PERSON;
    value="TableName,TraceID";
    limits="PersonID="+to_string(PersonID)+" AND PersonModule="+to_string(PersonModule)+" AND TraceID IS NOT NULL";
    //搜索对应人员在人员表中记录的最近一条记录
    string_table ret=DB.selectItem(table,value,limits);
    if(ret.size()==0){
        return DB_RET_PERSON_ERROR;
    }else if(ret.size()==1){
        table=ret[0][0];
        value="*";
        limits="TraceID="+ret[0][1];
        string_table temp=DB.selectItem(table,value,limits);
        if(temp.size()==0){
            return DB_RET_NULL;
        }
        pTrace=pTrace.readTrace(temp);
    }
    return DB_RET_OK;
}
//搜索某几个人员最近一条的轨迹信息
/*
    Trace用来回传轨迹信息将会包含（TraceID，PersonID,PersonModule,DeviceID,X,Y,Floor,MapMark,Time)
    返回值：
    DB_RET_OK成功查询轨迹
    DB_RET_NULL需查询设备无最近轨迹记录
*/
int DBTraceAPI::DBSearchPerson(const vector<vector<int>>&Person,vector<DBTrace>&Trace){
    DB.useDB(database);
    for(auto &v:Person){
        DBTrace tempTrace;
        if(DBSearchPerson(v[0],v[1],tempTrace)==DB_RET_OK){
            Trace.push_back(tempTrace);
        }
    }
    if(Trace.size()==0){
        return DB_RET_NULL;
    }else{
        return DB_RET_OK;
    }
}
//搜索所有人员最近一条的轨迹信息
/*
    Traces用来回传轨迹信息将会包含（TraceID，PersonID,PersonModule,DeviceID,X,Y,Floor,MapMark,Time)
    返回值：
    DB_RET_OK成功查询轨迹
    DB_RET_NULL需查询设备无最近轨迹记录
*/
int DBTraceAPI::DBSearchPerson (vector<DBTrace>&Traces){
    DB.useDB(database);
    string_table tablename;
    table=BASE_TABLE_PERSON;
    value="TableName,TraceID";
    limits="TraceID IS NOT NULL";
    //获取所有人员最后一条轨迹的位置
    tablename=DB.selectItem(table,value,limits);
    //int rows=tablename.size();
    if(tablename.empty()){
        return DB_RET_PERSON_ERROR;
    }
    value="*";
    for(auto &v:tablename){
        table=v[0];
        limits="TraceID="+v[1];
        string_table temp=DB.selectItem(table,value,limits);
        DBTrace trace;
        trace=trace.readTrace(temp);
        Traces.push_back(trace);
    }
    if(Traces.size()==0){
        return DB_RET_NULL;
    }else{
        return DB_RET_OK;
    }
}
//搜索时间区间内的人员轨迹信息
/*
    Traces用来回传轨迹信息将会包含（TraceID，PersonID,PersonModule,DeviceID,X,Y,Floor,MapMark,Time)
    返回值：
    DB_RET_OK成功查询轨迹
    DB_RET_NULL时间区间内无轨迹
*/
int DBTraceAPI::DBSearchPersonTrace(int PersonID,int PersonModule,ptime timeBegin,ptime timeEnd,vector<DBTrace>&Traces){
    DB.useDB(database);
    string tracetable1,tracetable2;
    table=BASE_TABLE_TRACE;
    value="TableName";
    string timeB=ptime_to_string(timeBegin);
    string timeE=ptime_to_string(timeEnd);
    //string timeB=to_iso_extended_string(timeBegin);
    //timeB=timeB.replace(timeB.find("T"),1," ");
    //string timeE=to_iso_extended_string(timeEnd);
    //timeE=timeE.replace(timeE.find("T"),1," ");
    limits="YearMonth>="+timeB.substr(0,4)+timeB.substr(5,2)+" AND YearMonth<="+timeE.substr(0,4)+timeE.substr(5,2);
    //检索在时间区间内的所有轨迹表
    string_table ret=DB.selectItem(table,value,limits);
    //轨迹表为0则返回没有数据，不为0则开始搜索
    //int rows=ret.size();
    if(ret.empty()){
        return DB_RET_NULL;
    }
    string_table temp;
    value="*";
    limits="PersonID="+to_string(PersonID)+" AND PersonModule="+to_string(PersonModule)+" AND Time>='"+timeB+"' AND Time<='"+timeE+"'";
    for(auto &v:ret){
        table=v[0];
        temp=DB.selectItem(table,value,limits);
        DBTrace trace;
        vector<DBTrace> res;
        res=trace.readTraces(temp);
        Traces.insert(Traces.end(),res.begin(),res.end());
    }
    if(Traces.size()==0){
        return DB_RET_NULL;
    }else{
        return DB_RET_OK;
    }
}
//搜索时间区间内的设备轨迹信息
/*
    Traces用来回传轨迹信息将会包含（TraceID，PersonID,PersonModule,DeviceID,X,Y,Floor,MapMark,Time)
    返回值：
    DB_RET_OK成功查询轨迹
    DB_RET_NULL时间区间内无轨迹
*/
int DBTraceAPI::DBSearchDeviceTrace(string DeviceID,ptime timeBegin,ptime timeEnd,vector<DBTrace>&Traces){
    DB.useDB(database);
    string tracetable1,tracetable2;
    table=BASE_TABLE_TRACE;
    value="TableName";
    string timeB=ptime_to_string(timeBegin);
    string timeE=ptime_to_string(timeEnd);
    //string timeB=to_iso_extended_string(timeBegin);
    //timeB=timeB.replace(timeB.find("T"),1," ");
    //string timeE=to_iso_extended_string(timeEnd);
    //timeE=timeE.replace(timeE.find("T"),1," ");
    limits="YearMonth>="+timeB.substr(0,4)+timeB.substr(5,2)+" AND YearMonth<="+timeE.substr(0,4)+timeE.substr(5,2);
    //检索在时间区间内的所有轨迹表
    string_table ret=DB.selectItem(table,value,limits);
    //轨迹表为0则返回没有数据，不为0则开始搜索
    //int rows=ret.size();
    if(ret.empty()){
        return DB_RET_NULL;
    }
    string_table temp;
    value="*";
    limits="DeviceID='"+DeviceID+"' AND Time>='"+timeB+"' AND Time<='"+timeE+"'";
    for(auto &v:ret){
        table=v[0];
        temp=DB.selectItem(table,value,limits);
        DBTrace trace; 
        vector<DBTrace> res;
        res=trace.readTraces(temp);
        Traces.insert(Traces.end(),res.begin(),res.end());
    }
    if(Traces.size()==0){
        return DB_RET_NULL;
    }else{
        return DB_RET_OK;
    }
}
//搜索时间区间内的设备轨迹信息
/*
    Traces用来回传轨迹信息将会包含（TraceID，PersonID,PersonModule,DeviceID,X,Y,Floor,MapMark,Time)
    返回值：
    DB_RET_OK成功查询轨迹
    DB_RET_NULL时间区间内无轨迹
*/
int DBTraceAPI::DBSearchTimeTrace(ptime timeBegin,ptime timeEnd,vector<DBTrace>&Traces){
    DB.useDB(database);
    string tracetable1,tracetable2;
    table=BASE_TABLE_TRACE;
    value="TableName";
    string timeB=ptime_to_string(timeBegin);
    string timeE=ptime_to_string(timeEnd);
    //string timeB=to_iso_extended_string(timeBegin);
    //timeB=timeB.replace(timeB.find("T"),1," ");
    //string timeE=to_iso_extended_string(timeEnd);
    //timeE=timeE.replace(timeE.find("T"),1," ");
    limits="YearMonth>="+timeB.substr(0,4)+timeB.substr(5,2)+" AND YearMonth<="+timeE.substr(0,4)+timeE.substr(5,2);
    //检索在时间区间内的所有轨迹表
    string_table ret=DB.selectItem(table,value,limits);
    //轨迹表为0则返回没有数据，不为0则开始搜索
    //int rows=ret.size();
    if(ret.empty()){
        return DB_RET_NULL;
    }
    string_table temp;
    value="*";
    limits="Time>='"+timeB+"' AND Time<='"+timeE+"'";
    for(auto &v:ret){
        table=v[0];
        temp=DB.selectItem(table,value,limits);
        DBTrace trace; 
        vector<DBTrace> res;
        res=trace.readTraces(temp);
        Traces.insert(Traces.end(),res.begin(),res.end());
    }
    if(Traces.size()==0){
        return DB_RET_NULL;
    }else{
        return DB_RET_OK;
    }
}
//搜索所有已存设备
/*
    DeviceID用来存储保存在数据库中的设备ID
    返回值：
    DB_RET_OK成功查询
    DB_RET_NULL无设备
*/
int DBTraceAPI::DBSearchDeviceID(vector<string>&DeviceID){
    DB.useDB(database);
    table=BASE_TABLE_DEVICE;
    value="DeviceID";
    //搜索设备表，获取设备ID
    string_table ret=DB.selectItem(table,value);
    if(ret.size()==0){
        return DB_RET_NULL;
    }
    for(auto &v:ret){
        DeviceID.push_back(v[0]);
    }

    return DB_RET_OK;
}
//删除单条轨迹信息
/*
    trace至少提供轨迹保存时间time信息和TraceID信息
    返回值：
    DB_RET_OK删除轨迹成功
    DB_RET_NULL未找到轨迹所在轨迹表
    DB_RET_FALL删除失败
*/
int DBTraceAPI::DBDeleteTrace(const DBTrace&trace){
    DB.useDB(database);
    table=BASE_TABLE_TRACE;
    value="TableName";
    limits="YearMonth='"+trace.time.substr(0,4)+trace.time.substr(5,2)+"'";
    //搜索轨迹所在表格
    string_table ret=DB.selectItem(table,value,limits);
    if(ret.size()==0){
        return DB_RET_NULL;
    }
    table=ret[0][0];
    limits="TraceID="+to_string(trace.TraceID);
    //删除轨迹
    if(DB.deleteItem(table,limits)){
        return DB_RET_OK;
    }else{
        return DB_RET_FALL;
    }
}
//删除设备以及其对应关系
/*
    返回值：
    DB_RET_OK删除成功
    DB_RET_FALL删除失败
*/
int DBTraceAPI::DBDeleteDevice(string DeviceID){
    DB.useDB(database);
    table=BASE_TABLE_DEVICE;
    limits="DeviceID='"+DeviceID+"'";
    DB.autoCommitOff();
    bool flag_delete=true;
    //删除在设备表中的信息
    if(DB.deleteItem(table,limits)==false){
        flag_delete=false;
    }
    table=BASE_TABLE_DEVICE_PERSON_RELATE;
    //删除在设备人员关系表中的信息
    if(DB.deleteItem(table,limits)==false){
        flag_delete=false;
    }
    //若失败则部提交事件，事件回滚保持原有状态
    if(flag_delete==true){
        DB.commit();
        return DB_RET_OK;
    }else{
        DB.rollback();
        return DB_RET_FALL;
    }
}
//删除围栏信息
/*
    返回值：
    DB_RET_OK删除成功
    DB_RET_FALL删除失败
*/
int DBTraceAPI::DBDeleteMap(int MapMark){
    DB.useDB(database);
    table=BASE_TABLE_MAP;
    limits="MapMark="+to_string(MapMark);
    if(DB.deleteItem(table,limits)){
        return DB_RET_OK;
    }else{
        return DB_RET_FALL;
    }
}
//更新轨迹信息
/*
    返回值：
    DB_RET_OK更新成功
    DB_RET_FALL更新失败
    DB_RET_NULL检索不到原轨迹所在表
*/
int DBTraceAPI::DBUpdateTrace(const DBTrace&traceOld,const DBTrace&traceNew){
    DB.useDB(database);
    table=BASE_TABLE_TRACE;
    value="TableName";
    limits="YearMonth="+traceOld.time.substr(0,4)+traceOld.time.substr(5,2);
    //搜索该条轨迹说在表格
    string_table ret=DB.selectItem(table,value,limits);
    if(ret.size()==0){
        return DB_RET_NULL;
    }
    table=ret[0][0];
    value="PersonID="+to_string(traceNew.PersonID)+", PersonModule="+to_string(traceNew.PersonModule)+", DeviceID=\""+traceNew.DeviceID+"\", X="+to_string(traceNew.X)+", Y="+to_string(traceNew.Y)+", Floor=\""+traceNew.Floor+"\", MapMark="+to_string(traceNew.MapMark)+", time=\""+traceNew.time+"\"";
    limits="TraceID="+to_string(traceOld.TraceID);
    if(DB.updateItem(table,value,limits)){
        return DB_RET_OK;
    }else{
        return DB_RET_FALL;
    }
}
//清除所有轨迹表
/*
    返回值：
    DB_RET_OK清除成功
    DB_RET_FALL清除失败
    DB_RET_NULL Trace表为空
*/
int DBTraceAPI::DBClearTable(){
    DB.useDB(database);
    table=BASE_TABLE_TRACE;
    value="distinct TableName";
    //获取所有轨迹表名
    string_table ret=DB.selectItem(table,value);
    if(ret.size()==0){
        return DB_RET_NULL;
    }
    //批量删除
    DB.autoCommitOff();
    bool flag_delete=true;
    //for(unsigned int i=0;i<ret.size();i++){
    for(auto &v:ret){
        if(DB.deleteTB(v[0])==false){
            flag_delete=false;
            break;
        }
    }
    if(DB.clearTB(table)==false){
        flag_delete=false;
    }
    if(flag_delete){
        DB.commit();
        return DB_RET_OK;
    }else{
        DB.rollback();
        return DB_RET_FALL;
    }
}
//统计进出次数和持续时间方法
DBMapData CountFre(vector<DBTrace>&trace,int PersonID,int PersonModule,int MapMark){
    int flagOld=0;
    int flagNew=0;
    DBMapData tempMapData(PersonID,PersonModule,MapMark);
    for(unsigned int i=0;i<trace.size();i++){
        if(trace[i].PersonID==PersonID&&trace[i].PersonModule==PersonModule){
            if(trace[i].MapMark==MapMark){
            flagNew=1;
        }else{
            flagNew=0;
        }
        if(flagNew==1 && flagOld==0){
            tempMapData.Enter++;
        }else if (flagNew==0 && flagOld==1){
            ptime temp = (time_from_string(trace[i].time)+tempMapData.StayTime);
            tempMapData.StayTime=temp-time_from_string(trace[i-1].time);
            tempMapData.Out++;
        }else if (flagNew==1 && flagOld==1){
            ptime temp = (time_from_string(trace[i].time)+tempMapData.StayTime);
            tempMapData.StayTime=temp-time_from_string(trace[i-1].time);
            tempMapData.Out++;
        }
        flagOld=flagNew;
        } 
    }
    return tempMapData;
}
//具体围栏数据统计
/*
    MapData用来返回统计的数据包含（PersonID，PersonModule，MapMark，Enter，Out，StayTime）
    返回值：
    DB_RET_OK统计成功
    DB_RET_FALL统计失败
    DB_RET_NULL时间段内的轨迹数据为空
*/
int DBTraceAPI::DBMapCount(int PersonID,int PersonModule,int MapMark,ptime timeBegin,ptime timeEnd,DBMapData&MapData){
    DB.useDB(database);
    vector<DBTrace> Traces;
    int res=DBSearchPersonTrace(PersonID,PersonModule,timeBegin,timeEnd,Traces);
    MapData.initData(PersonID,PersonModule,MapMark);
    if(res==DB_RET_OK){
        MapData=CountFre(Traces,PersonID,PersonModule,MapMark);
    }else if (res==DB_RET_NULL){
        return DB_RET_NULL;
    }else{
        return DB_RET_FALL;
    }
    return DB_RET_OK;
}
//具体人员在围栏位置的统计（输出对每隔围栏的统计）
/*
    MapData用来返回统计的数据包含（PersonID，PersonModule，MapMark，Enter，Out，StayTime）
    返回值：
    DB_RET_OK统计成功
    DB_RET_FALL统计失败
*/
int DBTraceAPI::DBMapPersonCount(int PersonID,int PersonModule,ptime timeBegin,ptime timeEnd,vector<DBMapData>&MapData){
    DB.useDB(database);
    vector<DBTrace> Traces;
    int res=DBSearchPersonTrace(PersonID,PersonModule,timeBegin,timeEnd,Traces);
    if(res==DB_RET_OK){
        table=BASE_TABLE_MAPMARK;
        value="MapMark";
        string_table ret=DB.selectItem(table,value);
        for(auto &v:ret){
            int MapMark=atoi(v[0].c_str());
            DBMapData tempMapData=CountFre(Traces,PersonID,PersonModule,MapMark);
            MapData.push_back(tempMapData);
        }

        return DB_RET_OK;
    }else{
        return DB_RET_FALL;
    }
    
}
//具体围栏的统计（输出对每个人的统计表）
/*
    MapData用来返回统计的数据包含（PersonID，PersonModule，MapMark，Enter，Out，StayTime）
    返回值：
    DB_RET_OK统计成功
    DB_RET_SEARCH_ERROR轨迹数据为空
    DB_RET_PERSON_ERROR人员数据为空
*/
int DBTraceAPI::DBMapMarkCount(int MapMark,ptime timeBegin,ptime timeEnd,vector<DBMapData>&MapData){
    DB.useDB(database);
    table=BASE_TABLE_PERSON;
    value="PersonID,PersonModule";
    string_table ret=DB.selectItem(table,value);
    if(ret.size()==0){
        return DB_RET_PERSON_ERROR;
    }
    vector<DBTrace> trace;
    if(DBSearchTimeTrace(timeBegin,timeEnd,trace)!=DB_RET_OK){
        return DB_RET_SEARCH_ERROR;
    }
    DBMapData tempMapData;
    for(auto &v:ret){
        tempMapData=CountFre(trace,atoi(v[0].c_str()),atoi(v[1].c_str()),MapMark);
        MapData.push_back(tempMapData);
    }

    return DB_RET_OK;
}
//统计围栏的总进出数和持续时间
/*
    MapData用来返回统计的数据包含（MapMark，rate，StayTime）
    返回值：
    DB_RET_OK统计成功
    DB_RET_FALL统计失败
*/
int DBTraceAPI::MapMarkCount(int MapMark,ptime timeBegin,ptime timeEnd,DBMapData&MapData){
    vector<DBMapData> tempData;
    int rec=DBMapMarkCount(MapMark,timeBegin,timeEnd,tempData);
    if(rec!=DB_RET_OK){
        return DB_RET_FALL;
    }
    MapData.MapMark=MapMark;
    MapData.rate=0;
    for(auto &v:tempData){
        MapData.rate=MapData.rate+v.Enter+v.Out;
        MapData.StayTime=(MapData.StayTime+v.StayTime);
    }

    return DB_RET_OK;
}
//统计所有围栏在该时间段的数据
/*
    MapData用来返回统计的数据包含（MapMark，rate，StayTime）
    返回值：
    DB_RET_OK统计成功
    DB_RET_FALL统计失败
*/
int DBTraceAPI::MapCount(ptime timeBegin,ptime timeEnd,vector<DBMapData>&MapData){
    DB.useDB(database);
    table=BASE_TABLE_MAPMARK;
    value="MapMark";
    string_table ret=DB.selectItem(table,value);
    if(ret.size()==0){
        return DB_RET_NULL;
    }
    for(auto &v:ret){
        DBMapData tempData;
        if(MapMarkCount(atoi(v[0].c_str()),timeBegin,timeEnd,tempData)==DB_RET_OK){
            MapData.push_back(tempData);
        }
    }

    return DB_RET_OK;
}
//删除数据库
/*
    返回值：
    DB_RET_OK删除成功
    DB_RET_FALL删除失败
*/
int DBTraceAPI::DBDeleteDB(){
    if(DB.deleteDB(database)){
        return DB_RET_OK;
    }else{
        return DB_RET_FALL;
    }
}