
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

#include "cereal/archives/binary.hpp"
#include "cereal/types/memory.hpp"
#include "cereal/types/base_class.hpp"
#include "cereal/types/vector.hpp"


// http://pcjericks.github.io/py-gdalogr-cookbook/index.html
// https://github.com/Tencent/rapidjson
// http://pcjericks.github.io/py-gdalogr-cookbook/geometry.html#create-geometry-from-wkb
using namespace std;

struct relation_geom
{
    int id;
    int type;
    int parent;
    int color;
    string geom;

    template <class Archive>
    void save(Archive& ar) const
    {
        ar(id, type, parent, color, geom);
    }

    template <class Archive>
    void load(Archive& ar)
    {
        ar(id, type, parent, color, geom);
    }
};

struct s_geom
{
    string geom_type;
    vector<relation_geom> v_geom;

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
    string geom;
    string buffer;
    string left_buff;
    string right_buff;

    edge_relation()
    {
        have_relation = 0;
    }

    edge_relation(const relation_geom& geom)
    {
        this->id = geom.id;
        this->color = geom.color;
        this->type = geom.type;
        this->parent = geom.parent;
        this->geom = geom.geom;
    }

    edge_relation& operator&=(const relation_geom& geom)
    {
        this->id = geom.id;
        this->color = geom.color;
        this->type = geom.type;
        this->parent = geom.parent;
        return *this;
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

void save_wkt_to_cereal(const s_geom& geom, string& cereal_path)
{
    ofstream out0;
    {
        out0.open(cereal_path, std::ios::binary);
        cereal::BinaryOutputArchive archive(out0);
        archive(geom);
    }
    out0.close();
}

void load_wkt_from_cereal(s_geom& geom, const string& cereal_path)
{
    ifstream in0;
    {
        in0.open(cereal_path, ios::in);
        cereal::BinaryInputArchive archive(in0);
        archive(geom);
    }
    in0.close();
}

void get_shp_buffer_from_wkt( std::vector<edge_relation>& v_wkt_geom, const double& distance )
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
    //params.setSingleSided(true); // DO NOT switch for non-areal input, see ticket #633
    BufferBuilder builder(params);

    string s;
    for(auto& x: v_wkt_geom){
        if(flag == "left"){
            auto geo = reader_.read(x.geom);
            GeomPtr gB(builder.bufferLineSingleSided(geo.get(), distance, true));
            x.left_buff = gB->toString();
        }

        // right-side
        else if(flag == "right"){
            auto geo = reader_.read(x.geom);
            GeomPtr gB(builder.bufferLineSingleSided(geo.get(), distance, false));
            x.right_buff = gB->toString();
        }
    }
}

void delete_contain_geoms( std::vector<relation_geom>& v_origin_geom, const std::vector<relation_geom>& v_buff_geom,  std::vector<relation_geom>& v_geom )
{
    typedef std::unique_ptr<geos::geom::LineString> LineStringAutoPtr;
    typedef geos::geom::LineString* LineStringPtr;
    typedef geos::geom::Geometry::Ptr GeomAutoPtr;
    typedef geos::geom::Polygon* PolygonPtr;

    geos::io::WKTReader reader_;
    
    LineStringPtr empty_line_;
    PolygonPtr polygon_;

    for(auto&x: v_buff_geom)
    {
        auto geo = reader_.read(x.geom);
        assert(geo != nullptr);

        polygon_ = dynamic_cast<PolygonPtr>(geo.get());
        // cout<<polygon_->getGeometryType()<<endl;

        for(auto it=v_origin_geom.begin(); it!=v_origin_geom.end(); ){
            auto line_ = reader_.read(it->geom);
            empty_line_ = dynamic_cast<LineStringPtr>(line_.get());
            // cout<<empty_line_->toString()<<endl;

            if( polygon_->contains(empty_line_) && x.id != it->id ){
                cout<<"delete line"<<endl;
                it = v_origin_geom.erase(it);
            }else{
                ++it;
            }
        }
    }
    v_geom.insert(v_geom.end(), v_origin_geom.begin(), v_origin_geom.end());
}

void transform_wkt_from_geojson( const std::vector<relation_geom>& v_geojson_geom, std::vector<relation_geom>& v_wkt_geom )
{
    char* wkt = new char[1024];
    for(auto&x: v_geojson_geom)
    {
        relation_geom data ;
        OGRGeometry *geo;
        geo = OGRGeometryFactory::createFromGeoJson( x.geom.c_str() );
        geo->exportToWkt( &wkt );
        // cout<<wkt<<endl;
        data.id = x.id;
        data.geom = wkt;
        data.type = x.type;
        data.color = x.color;
        data.parent = x.parent;
        v_wkt_geom.push_back(data);
    }
    delete[] wkt;
}

void load_shp(const string& shp_path){
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

    poLayer->ResetReading();
    int i=0;

    for(int i =0;i< poLayer->GetFeatureCount(); i++ ){
        OGRFeature *poFeature1 = poLayer->GetFeature(205);
        // OGRGeometry *p1 = poFeature1->GetGeometryRef();
        // cout<<p1->IsSimple()<<endl;
    }

    while( (poFeature = poLayer->GetNextFeature()) != NULL )
    {
        // if(poFeature->GetFieldAsDouble("AREA")<1) continue; //去掉面积过小的polygon
        // i=i++;
        // cout<<i<<"  ";
        OGRFeatureDefn *poFDefn = poLayer->GetLayerDefn();
        int iField;
        int n=poFDefn->GetFieldCount(); //获得字段的数目，不包括前两个字段（FID,Shape);
        for( iField = 0; iField <n; iField++ )
        {
            //输出每个字段的值
            // cout<<poFeature->GetFieldAsString(iField)<<"    ";
        }
        // cout<<endl;
        OGRFeature::DestroyFeature( poFeature );
    }
    GDALClose( poDS );
}

// create shp from wkt
void create_shp_from_wkt(const string& shp_path, const string& geom_type, const std::vector<relation_geom>& v_geom_value)
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

    poDS = poDriver->Create( "/home/xiaolinz/uisee_src/topo_source/road_topo/shpdata/point_out.shp", 0, 0, 0, GDT_Unknown, NULL );
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
        // string geojson = R"({"type":"LineString","coordinates":[[1016247.90861206,3461292.298148833,12.865576057136304],[1016247.906561,3461292.416155,12.865353],[1016247.8676150001,3461294.3419499995,12.854049],[1016247.8218410001,3461296.2679470006,12.831742],[1016247.763157,3461298.194158,12.817692],[1016247.712768,3461300.1201929997,12.814971],[1016247.685167,3461302.046,12.818],[1016247.659925,3461303.972771,12.82819],[1016247.645309,3461305.899463,12.830226],[1016247.628803,3461307.82531,12.83351],[1016247.603077,3461309.752077,12.837474],[1016247.579599,3461311.678187,12.844675],[1016247.600022,3461313.605616,12.851178],[1016247.622209,3461315.532386,12.858293],[1016247.583385,3461317.458946,12.860758],[1016247.5333280001,3461319.384925,12.852623],[1016247.5367690001,3461321.311692,12.852999],[1016247.55585,3461323.2384630006,12.840665],[1016247.578154,3461325.1658169997,12.83799],[1016247.7065904386,3461329.631826353,0.644264217964249]]})";
        geo = OGRGeometryFactory::createFromGeoJson( x.c_str() );
        cout<<geo->getGeometryType()<<endl;
        // cout<<x<<endl;
        poFeature->SetGeometry(geo);

        // save geom from geojson
        // OGRFeature* poFeature ;
        // OGRGeometry* geo ;
        // OGRSpatialReference *myespEPSG4326osr = new OGRSpatialReference("");
	    // myespEPSG4326osr->SetWellKnownGeogCS("EPSG:4326");

        // poFeature = OGRFeature::CreateFeature( poLayer->GetLayerDefn() );
        // poFeature->SetField( "Name", "aa" );

        // OGRGeometryFactory::createFromWkt( x.c_str(), NULL, &geo );
        // poFeature->SetGeometry( geo );
        
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

void load_geojson_rapidjson(const char* geojson,  string& geom_type, std::vector<relation_geom>& v_geom_value )
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
                relation_geom data;
                cout<<transform_dom_to_string(e)<<endl;
                
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
                // cout<<"90909009"<<endl;

                if(e.HasMember("geometry"))
                {
                    data.id = i;
                    geom_type = e["geometry"]["type"].GetString();
                    rapidjson::Value &z = e["geometry"];
                    string geom_value = transform_dom_to_string(z);

                    data.geom = geom_value;
                    v_geom_value.push_back(data);
                    ++i;
                }
            }
        }
    }
    cout<<"111111111"<<endl;
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

        std::cout<<" feature count "<<count <<std::endl;
        for(int i=0; i < count; i++)
        {
            JSON_Value *js_node_value =json_array_get_value(js_feature_list, i);
            JSON_Object *js_node = json_value_get_object(js_node_value);
            const char* name = json_object_get_string(js_node, "type");
            std::cout<<"type name "<< name <<std::endl;
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

            cout<<json_value_get_type(js_geo_node_value)<<endl;

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

void delete_short_lane(std::vector<relation_geom>& v_value)
{
    typedef std::unique_ptr<geos::geom::LineString> LineStringAutoPtr;
    typedef geos::geom::LineString* LineStringPtr;
    typedef geos::geom::Geometry::Ptr GeomAutoPtr;
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

void transform_cereal_to_relation(const std::vector<relation_geom>& v_value, std::vector<edge_relation>& v_geom)
{
    for(auto&x: v_value)
    {
        edge_relation relation(x);
        v_geom.push_back(relation);
    }
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
                    if( ll->distance( left_buff_line.get()) < 1 ){
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
    string shp_path = "/home/xiaolinz/uisee_src/topo_source/data/result.shp";
    string geo_json = "/home/xiaolinz/uisee_src/topo_source/data/lane_edge_fit.geojson";
    string cereal_path = "/home/xiaolinz/uisee_src/topo_source/data/result.bin";

    std::vector<relation_geom> v_geom_value;
    std::vector<relation_geom> v_wkt_value;
    std::vector<relation_geom> v_buff_geom;
    std::vector<relation_geom> v_result_geom;
    std::vector<geos::geom::LineString*> v_geos_value;
    string geom_type;
    //  geojson_file -> geo_string
    // load_geojson_rapidjson(geo_json.c_str(), geom_type, v_geom_value);

    // create_shp_from_geojson(shp_path, geom_type, v_geom_value);
    // geo_string -> wkt_string
    // transform_wkt_from_geojson(v_geom_value, v_wkt_value);

    double distance = 0.3 ;
    // wkt_string -> buffer_string
    // get_shp_buffer_from_wkt(v_wkt_value, v_buff_geom, distance);

    // wkt + buf -> delete_result_string
    // delete_contain_geoms(v_wkt_value, v_buff_geom, v_result_geom);

    // result_value -> delete short lane
    // delete_short_lane( v_result_geom );

    // construct 4m buffer for edge relation

    // get_shp_buffer_from_wkt(v_wkt_value, v_buff_geom, distance);
    // get_single_buff_from_wkt(v_wkt_value, v_buff_geom, distance, "left");

    geom_type = "LineString";
    s_geom geom;
    // geom.geom_type = geom_type;
    // geom.v_geom.assign(v_result_geom.begin(), v_result_geom.end());   

    
    // save_wkt_to_cereal(geom, cereal_path); 


    ////////////////////////////////////////////////////////////////

    load_wkt_from_cereal(geom, cereal_path);
    
    // create_shp_from_wkt(shp_path, geom_type, geom.v_geom);

    // geom_type = "Polygon";
    // create_shp_from_wkt(shp_path, geom_type, v_buff_geom);

    // TODO: make edge relationship 
    // std::vector<relation_geom> v_double_buff_geom;
    // std::vector<relation_geom> v_left_buff_geom;
    // std::vector<relation_geom> v_right_buff_geom;

    std::vector<edge_relation> v_geom;

    distance = 4.5;

    transform_cereal_to_relation(geom.v_geom, v_geom);

    get_shp_buffer_from_wkt( v_geom, distance );
    get_single_buff_from_wkt( v_geom, distance, "left" );
    get_single_buff_from_wkt( v_geom, distance, "right" );

    construct_edge_relation(v_geom);
    delete_error_relation(v_geom, distance);

    

    save_relation_to_shp( v_geom, "LineString", shp_path );

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