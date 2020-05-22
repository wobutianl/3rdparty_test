
#include "ogrsf_frmts.h"
#include <iostream>
#include "geos.h"
#include <map>
#include <vector>
#include "parson.h"
#include "ogr_geometry.h"
#include <fstream>
#include <iomanip>
#include <math.h>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include "geos/operation/buffer/BufferBuilder.h"
#include "geos/operation/buffer/BufferOp.h"
#include "geos/operation/buffer/BufferParameters.h"
#include "geos/algorithm/Orientation.h"
#include "geos/geomgraph/DirectedEdge.h"

#include "cereal/archives/binary.hpp"
#include "cereal/types/memory.hpp"
#include "cereal/types/base_class.hpp"
#include "cereal/types/vector.hpp"


// http://pcjericks.github.io/py-gdalogr-cookbook/index.html
// https://github.com/Tencent/rapidjson
// http://pcjericks.github.io/py-gdalogr-cookbook/geometry.html#create-geometry-from-wkb
using namespace std;


struct edge_relation
{
    int id;
    vector<int> left_edge;
    vector<int> right_edge;
    set<int> del_edges;
    int color;
    int parent;
    int type;
    int have_relation;
    bool found;
    int orientation; // 1: clock_wise, 2:counter_clock_wise
    int segment;
    int direction;
    int sequence;
    string geojson;
    string geom;
    string buffer;
    string left_buff;
    string right_buff;
    string left_buff_line;
    string right_buff_line;

    template <class Archive>
    void save(Archive& ar) const
    {
        ar(id, type, parent, color, geojson, geom, buffer, left_buff, right_buff, left_edge, right_edge);
    }

    template <class Archive>
    void load(Archive& ar)
    {
        ar(id, type, parent, color, geojson, geom, buffer, left_buff, right_buff, left_edge, right_edge);
    }

    edge_relation()
    {
        have_relation = 0;
        found = false;
        sequence = -1;
    }
};

struct s_geom
{
    string geom_type;
    vector<edge_relation> v_geom;

    template <class Archive>
    void save(Archive& ar) const
    {
        ar(geom_type, v_geom);
    }

    template <class Archive>
    void load(Archive& ar)
    {
        ar(geom_type, v_geom);
    }
};

void save_relation_to_shp(const vector<edge_relation>& v_relation, const string& geom_type, const string& shp_path)
{
    const char *pszDriverName = "ESRI Shapefile";
    GDALDriver *poDriver;

    GDALAllRegister();

    poDriver = GetGDALDriverManager()->GetDriverByName(pszDriverName );
    if( poDriver == NULL )
    {
        printf( "%s driver not available.\n", pszDriverName );
        exit( 1 );
    }

    GDALDataset *poDS;

    poDS = poDriver->Create( shp_path.c_str() , 0, 0, 0, GDT_Unknown, NULL );
    if( poDS == NULL )
    {
        printf( "Creation of output file failed.\n" );
        exit( 1 );
    }

    OGRLayer *poLayer;
    // const int wkbPoint = 1;
    // const int wkbLineString = 2;
    // const int wkbPolygon = 3;
    // const int wkbMultiPoint = 4;
    // const int wkbMultiLineString = 5;
    // const int wkbMultiPolygon = 6;
    // const int wkbGeometryCollection = 7;
    if(geom_type == "POINT"){
        poLayer = poDS->CreateLayer( "point_out", NULL, wkbPoint, NULL );
    }
    else if (geom_type == "LineString"){
        poLayer = poDS->CreateLayer( "line_out", NULL, wkbLineString, NULL );
    }
    else if (geom_type == "Polygon"){
        poLayer = poDS->CreateLayer( "polygon_out", NULL, wkbPolygon, NULL );
    }

    if( poLayer == NULL )
    {
        printf( "Layer creation failed.\n" );
        exit( 1 );
    }

    OGRFieldDefn type_field( "type", OFTInteger );
    OGRFieldDefn id_field ("id", OFTInteger);
    OGRFieldDefn parent_field( "parent", OFTInteger );
    OGRFieldDefn color_field ("color", OFTInteger);
    OGRFieldDefn rel_field ("relation", OFTInteger);

    OGRFieldDefn left_field ("left", OFTString );
    OGRFieldDefn right_field ("right", OFTString );
    OGRFieldDefn del_field ("del", OFTString );

    OGRFieldDefn seq_field ("seq", OFTInteger );

    poLayer->CreateField( &type_field );
    poLayer->CreateField( &id_field );
    poLayer->CreateField( &parent_field );
    poLayer->CreateField( &color_field );
    poLayer->CreateField( &rel_field );

    poLayer->CreateField( &left_field );
    poLayer->CreateField( &right_field );
    poLayer->CreateField( &del_field );
    poLayer->CreateField( &seq_field );

    for(const auto&x: v_relation){
        // save geom from wkt
        OGRFeature* poFeature ;
        OGRGeometry* geo ;
        OGRSpatialReference *utm51n = new OGRSpatialReference("");
	    utm51n->SetWellKnownGeogCS("EPSG:32651");

        poFeature = OGRFeature::CreateFeature( poLayer->GetLayerDefn() );
        poFeature->SetField( "id", x.id );
        poFeature->SetField( "type", x.type );
        poFeature->SetField( "parent", x.parent );
        poFeature->SetField( "color", x.color );
        poFeature->SetField( "relation", x.have_relation );

        string left, right, del;
        for(auto&y: x.left_edge){
            left += to_string(y);
            left +=",";
        }
        for(auto&y: x.right_edge){
            right += to_string(y);
            right +=",";
        }
        for(auto&y: x.del_edges){
            del += to_string(y);
            del +=",";
        }

        poFeature->SetField( "left", left.c_str() );
        poFeature->SetField( "right", right.c_str() );
        poFeature->SetField( "del", del.c_str() );
        poFeature->SetField( "seq", x.sequence );

        OGRGeometryFactory::createFromWkt( x.geom.c_str(), NULL, &geo );
        poFeature->SetGeometry( geo );
        
        if( poLayer->CreateFeature( poFeature ) != OGRERR_NONE )
        {
            printf( "Failed to create feature in shapefile.\n" );
            exit( 1 );
        }
        OGRFeature::DestroyFeature( poFeature );
    }
    GDALClose( poDS );
}

void save_wkt_to_cereal(const vector<edge_relation>& v_relation, const string& geom_type, string& cereal_path)
{
    s_geom geom;
    geom.geom_type = geom_type;
    geom.v_geom.assign(v_relation.begin(), v_relation.end()); 

    ofstream out0;
    {
        out0.open(cereal_path, std::ios::binary);
        cereal::BinaryOutputArchive archive(out0);
        archive(geom);
    }
    out0.close();
}

void load_wkt_from_cereal( vector<edge_relation>& v_relation, string& geom_type, const string& cereal_path)
{
    s_geom geom;

    ifstream in0;
    {
        in0.open(cereal_path, ios::in);
        cereal::BinaryInputArchive archive(in0);
        archive(geom);
    }
    in0.close();

    geom_type = geom.geom_type;
    v_relation.assign(geom.v_geom.begin(), geom.v_geom.end()); 
}

void get_buffer_from_wkt( std::vector<edge_relation>& v_wkt_geom, const double& distance )
{
    typedef std::unique_ptr<geos::geom::LineString> LineStringAutoPtr;
    typedef geos::geom::LineString* LineStringPtr;
    typedef geos::geom::Geometry::Ptr GeomAutoPtr;

    geos::io::WKTReader reader_;
    LineStringPtr empty_line_;
    
    for( auto&x: v_wkt_geom ){
        GeomAutoPtr gb;
        auto geo = reader_.read(x.geom);

        empty_line_ = dynamic_cast<LineStringPtr>(geo.get());

        // EndCapStyle : 
        // CAP_ROUND = 1,
        // CAP_FLAT = 2,
        // CAP_SQUARE = 3
        gb = empty_line_->buffer(distance, 8, 2);

        x.buffer = gb->toString() ;

    }
}

// flag: 0: left buff  1: right buff 
void get_single_buff_line_from_wkt( std::vector<edge_relation>& v_wkt_geom, const double& distance, const string& flag = "left" )
{
    using geos::operation::buffer::BufferBuilder;
    using geos::operation::buffer::BufferParameters;
    using geos::algorithm::Orientation;
    using geos::geom::LineString;

    typedef geos::geom::Geometry::Ptr GeomPtr;
    geos::io::WKTReader reader_;

    // std::string wkt0("LINESTRING(0 0, 50 -10, 10 10, 0 50, -10 10)");

    BufferParameters params;
    params.setEndCapStyle(BufferParameters::CAP_FLAT);
    params.setQuadrantSegments(8);
    params.setJoinStyle(BufferParameters::JOIN_MITRE);
    params.setMitreLimit(5.57F);
    //params.setSingleSided(true); // DO NOT switch for non-areal input, see ticket #633
    BufferBuilder builder(params);

    string s;
    for(auto& x: v_wkt_geom){
        if(flag == "left"){
            auto geo = reader_.read(x.geom);
            GeomPtr gB(builder.bufferLineSingleSided(geo.get(), distance, true));
            x.left_buff_line = gB->toString();
        }

        // right-side
        else if(flag == "right"){
            auto geo = reader_.read(x.geom);
            GeomPtr gB(builder.bufferLineSingleSided(geo.get(), distance, false));
            x.right_buff_line = gB->toString();
        }
    }
}


void get_single_buff_from_wkt( std::vector<edge_relation>& v_wkt_geom, const double& distance, const string& flag = "left" )
{
    using geos::operation::buffer::BufferBuilder;
    using geos::operation::buffer::BufferParameters;
    using geos::algorithm::Orientation;
    using geos::geom::LineString;

    typedef geos::geom::Geometry::Ptr GeomPtr;
    geos::io::WKTReader reader_;

    // std::string wkt0("LINESTRING(0 0, 50 -10, 10 10, 0 50, -10 10)");

    BufferParameters params;
    params.setEndCapStyle(BufferParameters::CAP_FLAT);
    params.setQuadrantSegments(8);
    params.setJoinStyle(BufferParameters::JOIN_MITRE);
    params.setMitreLimit(5.57F);
    params.setSingleSided(true);

    double dist = distance;

    if(flag == "right"){
        dist = -distance;
    }
    //params.setSingleSided(true); // DO NOT switch for non-areal input, see ticket #633
    // BufferBuilder builder(params);

    string s;
    int i = 0;

    for(auto& x: v_wkt_geom){
        if(flag == "left"){
            ++i;
            auto geo = reader_.read(x.geom);
            // !!! need build erverytime
            BufferBuilder builder(params);
            GeomPtr gB(builder.buffer(geo.get(), dist));
            x.left_buff = gB->toString();
        }

        // right-side
        else if(flag == "right"){
            auto geo = reader_.read(x.geom);
            BufferBuilder builder(params);
            GeomPtr gB(builder.buffer(geo.get(), dist));
            x.right_buff = gB->toString();
        }
    }
}

// TODO: just used for judge, not for real delete
// TODO: intersection one meter => have error
void delete_contain_geoms( std::vector<edge_relation>& v_geom)
{
    typedef geos::geom::LineString* LineStringPtr;
    typedef geos::geom::Polygon* PolygonPtr;

    geos::io::WKTReader reader_;
    
    LineStringPtr empty_line_;
    PolygonPtr polygon_;

    for(auto&x: v_geom)
    {
        auto geo = reader_.read(x.buffer);
        assert(geo != nullptr);

        polygon_ = dynamic_cast<PolygonPtr>(geo.get());
        // cout<<polygon_->getGeometryType()<<endl;

        for(auto it= v_geom.begin(); it!= v_geom.end(); ){
            auto line_ = reader_.read(it->geom);
            empty_line_ = dynamic_cast<LineStringPtr>(line_.get());
            // cout<<empty_line_->toString()<<endl;

            if( polygon_->contains(empty_line_) && x.id != it->id ){
                cout<<"delete line"<<endl;
                it = v_geom.erase(it);
            }else{
                ++it;
            }
        }
    }
}

void transform_wkt_from_geojson( std::vector<edge_relation>& v_geojson_geom )
{
    char* wkt = new char[1024];
    for(auto&x: v_geojson_geom)
    {
        OGRGeometry *geo;
        geo = OGRGeometryFactory::createFromGeoJson( x.geojson.c_str() );
        geo->exportToWkt( &wkt );
        x.geom = wkt;
    }
    delete[] wkt;
}

//字符串分割函数
std::vector<std::string> split( std::string str, std::string pattern )
{
    std::string::size_type pos;
    std::vector<std::string> result;
    str += pattern;//扩展字符串以方便操作
    int size = str.size();

    for(int i=0; i<size; i++)
    {
        pos = str.find( pattern, i );
        cout<< "pos : "<< pos << endl;
        if( pos<size )
        {
            std::string s=str.substr( i, pos-i );
            result.push_back(s);
            i = pos+pattern.size()-1;
        }
    }
    return result;
}

void load_shp(const string& shp_path, std::vector<edge_relation>& v_geom ){
    GDALAllRegister();
    GDALDataset   *poDS;
    CPLSetConfigOption("SHAPE_ENCODING","");  //解决中文乱码问题
    //读取shp文件
    poDS = (GDALDataset*) GDALOpenEx(shp_path.c_str(), GDAL_OF_VECTOR, NULL, NULL, NULL );
    
    if( poDS == NULL )
    {
        printf( "Open failed.\n%s" );
        return ;
    }

    OGRLayer  *poLayer;
    poLayer = poDS->GetLayer(0); //读取层
    OGRFeature *poFeature;
    OGRGeometry *pGeom;

    poLayer->ResetReading();
    int i=0;

    char* wkt = new char[1024];
    for(int i =0;i< poLayer->GetFeatureCount(); i++ ){
        edge_relation geom;

        poFeature = poLayer->GetFeature(i);
        pGeom = poFeature->GetGeometryRef();

        pGeom->exportToWkt(&wkt);
        geom.geom = wkt;

        geom.id = poFeature->GetFieldAsInteger("id");
        geom.color = poFeature->GetFieldAsInteger("color");

        geom.type = poFeature->GetFieldAsInteger("type");
        geom.parent = poFeature->GetFieldAsInteger("parent");

        string left_edge = poFeature->GetFieldAsString("left");
        string right_edge = poFeature->GetFieldAsString("right");
        string del_edges = poFeature->GetFieldAsString("del");

        std::vector<std::string> left = split(left_edge, ",");
        std::vector<std::string> right = split(right_edge, ",");
        std::vector<std::string> del = split(del_edges, ",");

        for( auto&x: left){
            if( x!= "" ){
                geom.left_edge.push_back( stoi(x) );
            }
        }
        for( auto&x: right){
            if( x!= "" ){
                geom.right_edge.push_back( stoi(x) );
            }
        }
        for( auto&x: del){
            if( x!= "" ){
                geom.del_edges.insert( stoi(x) );
            }
        }
        v_geom.push_back(geom);
    }
    // OGRFeature::DestroyFeature( poFeature );

    // while( (poFeature = poLayer->GetNextFeature()) != NULL )
    // {
    //     geom.id = poFeature->GetFieldAsInteger("id");
    //     geom.color = poFeature->GetFieldAsInteger("color");

    //     geom.type = poFeature->GetFieldAsInteger("type");
    //     geom.parent = poFeature->GetFieldAsInteger("parent");

    //     // OGRFeatureDefn *poFDefn = poLayer->GetLayerDefn();
    //     // int iField;
    //     // int n=poFDefn->GetFieldCount(); //获得字段的数目，不包括前两个字段（FID,Shape);
    //     // for( iField = 0; iField <n; iField++ )
    //     // {
    //     //     //输出每个字段的值
    //     //     cout<<poFeature->GetFieldAsString(iField)<<"    ";
    //     // }
    //     // cout<<endl;
    //     OGRFeature::DestroyFeature( poFeature );
    // }
    GDALClose( poDS );
}

// create shp from wkt
void create_shp_from_wkt(const string& shp_path, const string& geom_type, const std::vector<edge_relation>& v_geom_value)
{
    const char *pszDriverName = "ESRI Shapefile";
    GDALDriver *poDriver;

    GDALAllRegister();

    poDriver = GetGDALDriverManager()->GetDriverByName(pszDriverName );
    if( poDriver == NULL )
    {
        printf( "%s driver not available.\n", pszDriverName );
        exit( 1 );
    }

    GDALDataset *poDS;

    poDS = poDriver->Create( shp_path.c_str() , 0, 0, 0, GDT_Unknown, NULL );
    if( poDS == NULL )
    {
        printf( "Creation of output file failed.\n" );
        exit( 1 );
    }

    OGRLayer *poLayer;
    // const int wkbPoint = 1;
    // const int wkbLineString = 2;
    // const int wkbPolygon = 3;
    // const int wkbMultiPoint = 4;
    // const int wkbMultiLineString = 5;
    // const int wkbMultiPolygon = 6;
    // const int wkbGeometryCollection = 7;
    if(geom_type == "POINT"){
        poLayer = poDS->CreateLayer( "point_out", NULL, wkbPoint, NULL );
    }
    else if (geom_type == "LineString"){
        poLayer = poDS->CreateLayer( "line_out", NULL, wkbLineString, NULL );
    }
    else if (geom_type == "Polygon"){
        poLayer = poDS->CreateLayer( "polygon_out", NULL, wkbPolygon, NULL );
    }

    if( poLayer == NULL )
    {
        printf( "Layer creation failed.\n" );
        exit( 1 );
    }

    OGRFieldDefn type_field( "type", OFTInteger );
    OGRFieldDefn id_field ("id", OFTInteger);
    OGRFieldDefn parent_field( "parent", OFTInteger );
    OGRFieldDefn color_field ("color", OFTInteger);

    poLayer->CreateField( &type_field );
    poLayer->CreateField( &id_field );
    poLayer->CreateField( &parent_field );
    poLayer->CreateField( &color_field );


    for(const auto&x: v_geom_value){
        // save geom from wkt
        OGRFeature* poFeature ;
        OGRGeometry* geo ;
        OGRSpatialReference *myespEPSG4326osr = new OGRSpatialReference("");
	    myespEPSG4326osr->SetWellKnownGeogCS("EPSG:4326");

        poFeature = OGRFeature::CreateFeature( poLayer->GetLayerDefn() );
        poFeature->SetField( "id", x.id );
        poFeature->SetField( "type", x.type );
        poFeature->SetField( "parent", x.parent );
        poFeature->SetField( "color", x.color );

        OGRGeometryFactory::createFromWkt( x.geom.c_str(), NULL, &geo );
        poFeature->SetGeometry( geo );
        
        if( poLayer->CreateFeature( poFeature ) != OGRERR_NONE )
        {
            printf( "Failed to create feature in shapefile.\n" );
            exit( 1 );
        }
        OGRFeature::DestroyFeature( poFeature );
    }
    GDALClose( poDS );
}


// create shp from geojson
void create_shp_from_geojson(const string& shp_path, const string& geom_type, const std::vector<string>& v_geom_value)
{
    const char *pszDriverName = "ESRI Shapefile";
    GDALDriver *poDriver;

    GDALAllRegister();

    poDriver = GetGDALDriverManager()->GetDriverByName(pszDriverName );
    if( poDriver == NULL )
    {
        printf( "%s driver not available.\n", pszDriverName );
        exit( 1 );
    }

    GDALDataset *poDS;

    poDS = poDriver->Create( shp_path.c_str(), 0, 0, 0, GDT_Unknown, NULL );
    if( poDS == NULL )
    {
        printf( "Creation of output file failed.\n" );
        exit( 1 );
    }

    OGRLayer *poLayer;
    // const int wkbPoint = 1;
    // const int wkbLineString = 2;
    // const int wkbPolygon = 3;
    // const int wkbMultiPoint = 4;
    // const int wkbMultiLineString = 5;
    // const int wkbMultiPolygon = 6;
    // const int wkbGeometryCollection = 7;
    if(geom_type == "POINT"){
        poLayer = poDS->CreateLayer( "point_out", NULL, wkbPoint, NULL );
    }
    else if (geom_type == "LineString"){
        poLayer = poDS->CreateLayer( "line_out", NULL, wkbLineString, NULL );
    }
    else if (geom_type == "Polygon"){
        poLayer = poDS->CreateLayer( "polygon_out", NULL, wkbPolygon, NULL );
    }

    if( poLayer == NULL )
    {
        printf( "Layer creation failed.\n" );
        exit( 1 );
    }

    OGRFieldDefn oField( "Name", OFTString );

    oField.SetWidth(32);

    if( poLayer->CreateField( &oField ) != OGRERR_NONE )
    {
        printf( "Creating Name field failed.\n" );
        exit( 1 );
    }

    for(const auto&x: v_geom_value){
        // save geom from wkt / geojson 
        OGRFeature *poFeature;
        poFeature = OGRFeature::CreateFeature( poLayer->GetLayerDefn() );
        poFeature->SetField( "Name", "aa" );
        OGRGeometry *geo;
        // string wkt = "POINT(108420.73 753808.99);";
        // string geojson = R"({"type":"LineString","coordinates":[[1016247.90861206,3461292.298148833]})";
        geo = OGRGeometryFactory::createFromGeoJson( x.c_str() );
        // cout<<geo->getGeometryType()<<endl;
        // cout<<x<<endl;
        poFeature->SetGeometry(geo);
        
        if( poLayer->CreateFeature( poFeature ) != OGRERR_NONE )
        {
            printf( "Failed to create feature in shapefile.\n" );
            exit( 1 );
        }
        OGRFeature::DestroyFeature( poFeature );
    }
    GDALClose( poDS );
}

string load_geojson( const char* geojson)
{
    string line, jsonText;
    ifstream ifs(geojson);
    while (getline(ifs, line))
        jsonText.append(line);
    return jsonText;
}

string transform_dom_to_string(const rapidjson::Value& json_value)
{
    // 3. 把 DOM 转换（stringify）成 JSON。
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    json_value.Accept(writer);
    return buffer.GetString();
}

void load_geojson_rapidjson(const char* geojson,  string& geom_type, std::vector<edge_relation>& v_geom_value )
{
    // string data = R"({ "errorCode":0, "reason":"OK", "result":{"userId":10086,"name":"中国移动"}, "numbers":[110,120,119,911] })";

    string data = load_geojson(geojson);
    rapidjson::Document d;

    if(d.Parse(data.c_str()).HasParseError()){
        printf("parse error!\n");
        return ;
    }
    if(!d.IsObject()){
        printf("should be an object!\n");
        return ;
    }
    int i = 1;
    if(d.HasMember("features")){
        rapidjson::Value &m = d["features"];

        if(m.IsArray()){
            const auto& array = m.GetArray();
            for( auto& e: array ){
                edge_relation data;
                // cout<<transform_dom_to_string(e)<<endl;
                
                if(e.HasMember("properties")){
                    auto &parent = e["properties"]["parent"];
                    auto &type = e["properties"]["type"];
                    auto &color = e["properties"]["color"];
                    
                    if( !parent.IsNull() ){
                        data.parent = parent.GetInt() ;
                    }else{
                        data.parent = -1;
                    }
                    if( !type.IsNull() ){
                        data.type = type.GetInt() ;
                    }else{
                        data.type = -1;
                    }
                    if( !color.IsNull() ){
                        data.color = color.GetInt() ;
                    }else{
                        data.color = -1;
                    }
                }
                if(e.HasMember("geometry"))
                {
                    data.id = i;
                    geom_type = e["geometry"]["type"].GetString();
                    rapidjson::Value &z = e["geometry"];
                    string geom_value = transform_dom_to_string(z);

                    data.geojson = geom_value;
                    v_geom_value.push_back(data);
                    ++i;
                }
            }
        }
    }
}

// https://blog.csdn.net/fuao/article/details/80950672?utm_source=blogxgwz8

// load geojosn 
// output: attribute , linestring
void load_geojson_parson(const char* geojson )
{
    std::map<std::string, std::vector<int>> attributes;
    // ID, Line 
    std::map<int, LineString*> medges;

    JSON_Value *data = json_parse_file(geojson);
    JSON_Object *obj_data = json_object(data);
    
    if(nullptr != data)
    {
        JSON_Value* js_feature_list_value = json_object_get_value(obj_data, "features");
        JSON_Array* js_feature_list = json_value_get_array(js_feature_list_value);
        int count = json_array_get_count(js_feature_list);

        // std::cout<<" feature count "<<count <<std::endl;
        for(int i=0; i < count; i++)
        {
            JSON_Value *js_node_value =json_array_get_value(js_feature_list, i);
            JSON_Object *js_node = json_value_get_object(js_node_value);
            const char* name = json_object_get_string(js_node, "type");
            // std::cout<<"type name "<< name <<std::endl;
            JSON_Value *js_prop_node_value = json_object_get_value(js_node, "properties");
            JSON_Object *js_prop_node = json_value_get_object(js_prop_node_value);
            int id = json_object_get_number(js_prop_node, "id");

            // if(id == 0)
            //     continue;
            attributes["id"].push_back(i);
            int class_type = json_object_get_number(js_prop_node, "type");
            attributes["class_type"].push_back(1);
            int parent = json_object_get_number(js_prop_node, "parent");
            attributes["parent"].push_back(parent);
            int color = json_object_get_number(js_prop_node, "color");
            attributes["color"].push_back(color);

            JSON_Value *js_geo_node_value = json_object_get_value(js_node, "geometry");

            // cout<<json_value_get_type(js_geo_node_value)<<endl;

            JSON_Object *js_geo_node = json_value_get_object(js_geo_node_value);

            JSON_Value *js_geo_coord_node_value = json_object_get_value(js_geo_node, "coordinates");
            JSON_Array *js_geo_coord_node = json_value_get_array(js_geo_coord_node_value);
            int size = json_array_get_count(js_geo_coord_node);

            CoordinateArraySequence * edge = new CoordinateArraySequence();
            for(int j=0; j < size; j++)
            {
                // Point2 p;
                JSON_Value *js_coord_node_value = json_array_get_value(js_geo_coord_node, j);
                JSON_Array *js_coord_node = json_value_get_array(js_coord_node_value);
                // p.x = json_array_get_number(js_coord_node, 0);
                // p.y = json_array_get_number(js_coord_node, 1);

                // edge->add(Coordinate(p.x,p.y));
            }
            // LineStringPtr ls= factory->createLineString(edge) ;
            // std::cout<<ls->getNumPoints()<<std::endl;
            // medges.emplace(i, ls);
        }
    }

}

void delete_short_lane(std::vector<edge_relation>& v_value)
{
    typedef geos::geom::LineString* LineStringPtr;
    typedef geos::geom::Polygon* PolygonPtr;

    geos::io::WKTReader reader_;
    
    LineStringPtr empty_line_;
    PolygonPtr polygon_;

    for(auto it=v_value.begin(); it!=v_value.end(); )
    {
        auto line_ = reader_.read( it->geom );
        empty_line_ = dynamic_cast<LineStringPtr>(line_.get());
        double lane_length = empty_line_->getLength();

        if( lane_length <= 1 ){
            cout<<"delete short line"<<endl;
            it = v_value.erase(it);
        }else{
            ++it;
        }
    }
}

geos::geom::Geometry::Ptr transform_wkt_to_geos(const string& wkt){
    typedef geos::geom::Geometry::Ptr GeomAutoPtr;
    geos::io::WKTReader reader_;
    GeomAutoPtr gb;
    auto geo = reader_.read(wkt);
    return geo;
}

void construct_edge_relation( std::vector<edge_relation>& v_geom )
{
    // vector<edge_relation> v_relation;
    // 1:buffer contain edge 
    // 2:edge distance to left edge buff line < 1m  then put it into left_edge
    for(auto&x : v_geom ){

        auto buffer_geos = transform_wkt_to_geos( x.buffer );

        for(auto&y: v_geom)
        {
            if( y.id == x.id) continue;

            auto ll = transform_wkt_to_geos(y.geom);

            if( buffer_geos->intersects( ll.get()) )
            {
                
                auto inter_geom = buffer_geos->intersection( ll.get() );
                auto left_buff_line = transform_wkt_to_geos( x.left_buff );
                // TODO: distance > 1m need judge 
                if( inter_geom->getLength() > 1)
                {
                    x.have_relation = 1;
                    if(  left_buff_line->intersects(ll.get() ) ){
                        x.left_edge.push_back(y.id);
                    }else{
                        x.right_edge.push_back(y.id);
                    }
                }
            }
        }
    }
}

void delete_error_relation( std::vector<edge_relation>& v_geom, const double& distance )
{
    // delete error relation
    for(auto&x: v_geom)
    {
        if(x.left_edge.size() + x.right_edge.size() >= 2)
        {
            // get all relations 
            vector<edge_relation> edge_wkt;
            auto base_geos = transform_wkt_to_geos(x.geom);

            // get all edge relation geoms
            for(const auto& y: x.left_edge )
            {
                for(const auto& z: v_geom)
                {
                    if( z.id == y )
                    {
                        edge_wkt.push_back(z);
                    }
                    
                }
            }
            for( const auto& y: x.right_edge )
            {
                for( const auto& z: v_geom)
                {
                    if( z.id == y )
                    {
                        edge_wkt.push_back(z);
                    }
                    
                }
            }

            // delete error relation
            for( auto y: edge_wkt )
            {
                auto origin_line = transform_wkt_to_geos( y.geom );
                auto buff = transform_wkt_to_geos( y.buffer );

                for( auto&z: edge_wkt )
                {
                    if( z.id == y.id ) continue;

                    auto match_line = transform_wkt_to_geos( z.geom );
                    auto interection_line = buff->intersection( match_line.get() );
                    if( interection_line->getLength() > 0.2 )
                    {
                        // if two line in left or right buffer and have intersect delete one 
                        auto left_geos = transform_wkt_to_geos( x.left_buff );
                        auto left_buff = left_geos->buffer(distance, 8, 2);

                        auto right_geos = transform_wkt_to_geos( x.right_buff );
                        auto right_buff = right_geos->buffer(distance, 8, 2);

                        if( left_buff->intersects( origin_line.get() ) && left_buff->intersects( match_line.get() ) )
                        {
                            if( base_geos->distance(origin_line.get() ) > base_geos->distance(match_line.get() ) )
                            {
                                x.del_edges.insert( y.id );
                            }
                            else
                            {
                                x.del_edges.insert( z.id );
                            }
                        }
                        else if( right_buff->intersects( origin_line.get() ) && right_buff->intersects( match_line.get() ) )
                        {
                            if( base_geos->distance(origin_line.get() ) > base_geos->distance(match_line.get() ) )
                            {
                                x.del_edges.insert( y.id );
                            }
                            else
                            {
                                x.del_edges.insert( z.id );
                            }
                        } 
                    }
                }
            }
            
        }
    }
}


void get_line_orientation( std::vector<edge_relation>& v_geom )
{
    typedef geos::geom::LineString::Ptr LineStringPtr;

    geos::io::WKTReader reader_;
    
    LineStringPtr empty_line_;
    CoordinateArraySequenceFactory csf; 
    std::unique_ptr<CoordinateSequence> cs = csf.create(1,2);

    // for(auto it=v_geom.begin(); it!=v_geom.end(); )
    // {
    //     auto line_ = reader_.read( it->geom );
    //     cs->setAt( *(line_.get()->getCoordinates()) , 0 ) ;
    //     if( geos::algorithm::Orientation::isCCW( cs.get() ) )
    //     {
    //         it->orientation = 2;
    //         cout<<"-----"<<it->orientation<<endl;
    //     }
    //     else
    //     {
    //         it->orientation = 1;
    //     }
    // }
}

void erase_delid_from_relation(std::vector<edge_relation>& v_geom)
{
    for(auto&x: v_geom)
    {
        for(const auto&y: x.del_edges){
            std::remove(std::begin(x.left_edge), std::end(x.left_edge), y);
            std::remove(std::begin(x.right_edge), std::end(x.right_edge), y) ;
        }
    }
}

void find_all_relation_ids( std::vector<edge_relation>& v_geom, set<int>& find, set<int>& match, vector<set<int> >& result  )
{
    if( find.empty() )
    {
        auto it = v_geom.begin();
        for(; it != v_geom.end(); ++it )
        {
            if( !it->found ) 
            {
                find.insert(it->id);
                cout<<"id insert into find "<<it->id<<endl;

                if( !match.empty() ) 
                {
                    // cout<<"input into result "<<endl;
                    result.push_back(match);
                    match.clear();
                }
                break;
            }        
        }
        if( it == v_geom.end() ) 
        {
            if( !match.empty() ) 
            {
                cout<<"input into result "<<endl;
                result.push_back(match);
                match.clear();
            }
            return;
        }
    }

    set<int> temp;
    for(auto&x: find)
    {
        // cout<<"current edge : "<<x<<endl;
        match.insert(x);

        for(auto&y: v_geom)
        {
            if( y.id == x )
            {
                // cout<<"----- match"<<endl;
                y.found = true;
                // cout<<"left edge size :"<<y.left_edge.size() <<endl;
                temp.insert( y.left_edge.begin(), y.left_edge.end() );
                temp.insert( y.right_edge.begin(), y.right_edge.end() );
                // cout<<"temp size is:"<<temp.size()<<endl;

                break;
            }
        }
        
    }
    find.clear();
    // find.insert(temp.begin(), temp.end() );
    for(auto&x: temp)
    {
        auto it = std::find( match.begin(), match.end(), x);
        if( it != match.end() ) continue;
        find.insert(x);
    }
    temp.clear();
    // if( find.empty() ) cout<<" find is empty "<<endl;

    return find_all_relation_ids(v_geom, find, match, result);
}

void set_segment( std::vector<edge_relation>& v_geom, const vector<set<int> >& result )
{
    int i = 1;
    for(auto&x : result)
    {
        for(auto&y: x)
        {
            for(auto&z: v_geom)
            {
                if(y == z.id)
                {
                    z.segment = i;
                }
            }
        }
        ++i;
    }
}

float getAngelOfTwoVector(const Point* sp1, const Point* ep1, const Point* sp2, const Point* ep2)
{
    const double PI = 3.141592653; 
	float theta = atan2(ep1->getX() - sp1->getX(), ep1->getY() - sp1->getY()) - 
        atan2( ep2->getX() - sp2->getX(), ep2->getY() - sp2->getY() );
	if (theta > PI)
		theta -= 2 * PI;
	if (theta < -PI)
		theta += 2 * PI;
 
	theta = theta * 180.0 / PI;
	return theta;
}

float get_two_vector_angle(const Point* sp1, const Point* ep1, const Point* sp2, const Point* ep2)
{
    const double PI = 3.141592653; 
    double vector1x = sp1->getX() - ep1->getX();
	double vector1y = sp1->getY() - ep1->getY();
	double vector2x = sp2->getX() - ep2->getX();
	double vector2y = sp2->getY() - ep2->getY();
	double t = ((vector1x)*(vector2x) + (vector1y)*(vector2y))/ (sqrt(pow(vector1x, 2) + 
                pow(vector1y, 2))*sqrt(pow(vector2x, 2) + pow(vector2y, 2)));
	cout << "这两个向量的夹角为:" << acos(t)*(180 / PI) << "度" << endl;
}

// true: same direction, false: other
bool is_same_direction(const Point* sp1, const Point* ep1, const Point* sp2, const Point* ep2)
{
    float angle = getAngelOfTwoVector(sp1, ep1, sp2, ep2);
    // float angle = get_two_vector_angle( sp1, ep1, sp2, ep2 );
    cout<<angle<<endl;
    if( angle <= 90  && angle >= -90 ) return true;
    return false;
}

void make_segment_relation( std::vector<edge_relation>& v_geom, std::map<int, std::vector<edge_relation*>>& m_relation )
{
    // std::map<int, std::vector<edge_relation*> > m_relation;  <segment: <int*> >
    for(auto&x: v_geom)
    {
        if( m_relation.find(x.segment) == m_relation.end() )
        {
            std::vector<edge_relation*> v_edge;
            v_edge.push_back(&x);
            m_relation.emplace(x.segment, v_edge);
        }
        else
        {
            m_relation[x.segment].push_back(&x);
        }
    }
}

void make_direction_relation( std::vector<edge_relation>& v_geom, std::map<int, std::map<int, std::vector<edge_relation*>> >& m_relation )
{
    // std::map<int, std::vector<edge_relation*> > m_relation;  <segment: <int*> >
    const int clock_wise = 1;
    const int under_clock_wise = 2;
    for(auto&x: v_geom)
    {
        if( m_relation.find(x.segment) == m_relation.end() )
        {
            std::map<int, std::vector<edge_relation*> > m_edge;
            std::vector<edge_relation*> v_dir1_edge;
            std::vector<edge_relation*> v_dir2_edge;
            if( x.direction == 1)
            {
                v_dir1_edge.push_back(&x);
            }
            else
            {
                v_dir2_edge.push_back(&x);
            }
            m_edge.emplace( 1, v_dir1_edge );
            m_edge.emplace( 2, v_dir2_edge );
            
            m_relation.emplace(x.segment, m_edge);
        }
        else
        {
            if( x.direction == 1)
            {
                m_relation[x.segment][1].push_back(&x);
            }
            else
            {
                m_relation[x.segment][2].push_back(&x);
            }
        }
    }
}

void cut_lines_into_two_direction( std::vector<edge_relation>& v_geom )
{
    typedef geos::geom::LineString* LineStringPtr;
    std::map<int, std::vector<edge_relation*> > m_rels;

    make_segment_relation(v_geom, m_rels);

    for(auto&x: m_rels)
    {
        if(x.second.size() > 0 )
        {
            x.second[0]->direction = 1;
            geos::geom::Geometry::Ptr base_line = transform_wkt_to_geos(x.second[0]->geom);
            LineStringPtr base_line_ = dynamic_cast<LineStringPtr>(base_line.get());

            std::unique_ptr<Point> base_p1 = base_line_->getPointN(1) ;
            std::unique_ptr<Point> base_p2 = base_line_->getPointN(3) ;

            for(auto&z: x.second)
            {
                if( x.second[0]->id == z->id ) continue;
                geos::geom::Geometry::Ptr match_line = transform_wkt_to_geos(z->geom);

                LineStringPtr match_line_ = dynamic_cast<LineStringPtr>(match_line.get());

                std::unique_ptr<Point> match_p1 = match_line_->getPointN(1) ;
                std::unique_ptr<Point> match_p2 = match_line_->getPointN(3) ;

                if( is_same_direction( base_p1.get(), base_p2.get(), match_p1.get(), match_p2.get() ) )
                {
                    z->direction = 1;
                }
                else
                {
                    z->direction = 2;
                }
            }
        }
    } 
}

void make_seq( std::vector<edge_relation>& v_geom, std::vector<std::vector<int> >& v_seq )
{
    std::map<int, std::map<int, std::vector<edge_relation*>> > m_relation;
    make_direction_relation( v_geom, m_relation );

    for( auto&seg: m_relation )
    {
        for(auto&x: seg.second)
        {
            std::vector<int> seq_info;
            seq_info.push_back( seg.first);
            seq_info.push_back(x.first);

            std::vector<int> find;
            for( auto&y: x.second )
            {   
                find.push_back(y->id);
            }

            int seq = 0;
            std::vector<int> v_ids;
            // first time : find all zero edge
            for( auto&z: x.second )
            {
                if( z->right_edge.empty() )
                {
                    cout<<"edge "<<z->id<<endl;
                    z->sequence = seq;
                    v_ids.insert( v_ids.end(), z->left_edge.begin(), z->left_edge.end() );
                    // v_ids.insert( v_ids.end(), z->right_edge.begin(), z->right_edge.end() );
                    find.erase( std::remove( find.begin(), find.end(), z->id ), find.end() );
                }
            }
            // find other edge
            std::vector<int> left_ids;
            while( !find.empty() )
            {
                cout<<"seq:"<<seq<<endl;
                ++seq;
                for( auto&y: v_ids )
                {
                    for( auto&z: x.second )
                    {
                        if( z->id == y )
                        {
                            z->sequence = seq;
                            // left_ids.insert( v_ids.end(), z->left_edge.begin(), z->left_edge.end() );
                            if( z->left_edge.size() > 0)
                            {
                                left_ids.insert( left_ids.end(), z->left_edge.begin(), z->left_edge.end() );
                                // v_ids.insert( v_ids.end(), z->right_edge.begin(), z->right_edge.end() );
                            }

                            find.erase( std::remove( find.begin(), find.end(), z->id ), find.end() );
                        }
                    }
                }
                v_ids.clear();
                v_ids.assign(left_ids.begin(), left_ids.end());
                left_ids.clear();
            }
            seq_info.push_back(seq);
            v_seq.push_back(seq_info);
        }
    }
}

void seq_adjust( std::vector<edge_relation>& v_geom, const std::vector<std::vector<int> >& v_seq)
{
    for(auto&x: v_seq)
    {
        for(auto&y: v_geom)
        {
            if(y.segment == x[0] && y.direction == x[1])
            {
                y.sequence = x[2] - y.sequence;
            }
        }
    }
}

int main()
{
    string input_shp_path = "/home/xiaolinz/uisee_src/topo_source/data/test2.shp";
    string shp_path = "/home/xiaolinz/uisee_src/topo_source/data/result.shp";
    string end_shp_path = "/home/xiaolinz/uisee_src/topo_source/data/result_end.shp";
    string geo_json = "/home/xiaolinz/uisee_src/topo_source/data/lane_edge_fit.geojson";
    string cereal_path = "/home/xiaolinz/uisee_src/topo_source/data/result.bin";

    std::vector<edge_relation> v_geom;
    string geom_type = "LineString";
    double distance = 4.5;

    
    
    // create wkt cereal
    {
        //  geojson_file -> string
        // load_geojson_rapidjson(geo_json.c_str(), geom_type, v_geom);
        
        // // geo_string -> wkt_string
        // transform_wkt_from_geojson(v_geom);

        // // result_value -> delete short lane
        // delete_short_lane( v_geom );
        
        // save_wkt_to_cereal(v_geom, geom_type, cereal_path); 
    }


    ////////////////////////////////////////////////////////////////
    

    {
        // TODO: make edge relationship 
        // load_wkt_from_cereal(v_geom, geom_type, cereal_path);

        load_shp(input_shp_path, v_geom);
        get_buffer_from_wkt( v_geom, distance );

        get_single_buff_from_wkt( v_geom, distance, "left" );
        get_single_buff_from_wkt( v_geom, distance, "right" );

        construct_edge_relation(v_geom);
        delete_error_relation(v_geom, distance);
        
        // save_relation_to_shp( v_geom, geom_type, shp_path );
    }

    // construct sequence
    {
        /*
        1: find the relation ids into vector< vector<int> > 
        2: cut ids into two direction 
        3: find edge lane mark as zero
        4: recure relation mark as 1,2,3
        5: reconstruct sequence
        */

        set<int> find;
        set<int> match;
        vector<set<int> > result;

        // load_shp(shp_path, v_geom);
        erase_delid_from_relation(v_geom);

        find_all_relation_ids(v_geom , find, match, result);

        set_segment(v_geom, result);
            
        cut_lines_into_two_direction(v_geom);

        std::vector<std::vector<int> > v_seq;
        make_seq(v_geom, v_seq);

        seq_adjust(v_geom, v_seq);

        save_relation_to_shp(v_geom,  geom_type, end_shp_path );

        for( auto&x: v_geom)
        {
            cout<<x.id<<"seq: == "<<x.sequence<<endl;
        }
    }

    // other function
    {

        // get_single_buff_line_from_wkt( v_geom, distance, "left" );
        // get_single_buff_line_from_wkt( v_geom, distance, "right" );

        // create_shp_from_geojson(shp_path, geom_type, v_geom_value);

        // geom_type = "Polygon";
        // create_shp_from_wkt(shp_path, geom_type, v_buff_geom);
    }

    // for(auto&x: v_relation){
    //     cout<<x.id <<":"<<endl;
    //     cout<<"left: ";
    //     for(auto&y: x.left_edge){
    //         cout<<y<<"  ";
    //     }
    //     cout<<endl<<"right: ";
    //     for(auto&y: x.right_edge){
    //         cout<<y<<"  ";
    //     }
    //     cout<<endl;
    // }

    
    system("pause");
    return 1;
}