
#include "ogrsf_frmts.h"
#include <iostream>
#include "geos.h"
#include <map>
#include <vector>
#include "parson.h"
#include "ogr_geometry.h"
#include <fstream>

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

    poLayer->CreateField( &type_field );
    poLayer->CreateField( &id_field );
    poLayer->CreateField( &parent_field );
    poLayer->CreateField( &color_field );
    poLayer->CreateField( &rel_field );

    poLayer->CreateField( &left_field );
    poLayer->CreateField( &right_field );
    poLayer->CreateField( &del_field );


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

int main()
{
    string input_shp_path = "/home/xiaolinz/uisee_src/topo_source/data/test_data.shp";
    string shp_path = "/home/xiaolinz/uisee_src/topo_source/data/result.shp";
    string geo_json = "/home/xiaolinz/uisee_src/topo_source/data/lane_edge_fit.geojson";
    string cereal_path = "/home/xiaolinz/uisee_src/topo_source/data/result.bin";

    std::vector<edge_relation> v_geom;
    load_shp(input_shp_path, v_geom);

    // create wkt cereal
    {
        string geom_type = "LineString";

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

        double distance = 4.5;
        string geom_type ;
        // load_wkt_from_cereal(v_geom, geom_type, cereal_path);

        get_buffer_from_wkt( v_geom, distance );
        // get_single_buff_line_from_wkt( v_geom, distance, "left" );
        // get_single_buff_line_from_wkt( v_geom, distance, "right" );

        get_single_buff_from_wkt( v_geom, distance, "left" );
        get_single_buff_from_wkt( v_geom, distance, "right" );

        construct_edge_relation(v_geom);
        delete_error_relation(v_geom, distance);
        
        // save_relation_to_shp( v_geom, "LineString", shp_path );
    }

    // construct sequence
    {


    }

    // other function
    {
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