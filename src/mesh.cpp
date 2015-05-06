#include "mesh.h"

#include <iostream>
#include "utils.h"
#include "vertexLayout.h"

Mesh::Mesh():m_drawMode(GL_TRIANGLES) {
    
}

Mesh::Mesh(const Mesh &_mother):m_drawMode(_mother.getDrawMode()) {
    add(_mother);
}

Mesh::~Mesh(){
    
}

bool Mesh::load(const std::string& _file) {
    if ( haveExt(_file,"ply") ){
        std::fstream is(_file.c_str(), std::ios::in);
        if(is.is_open()){
            
            std::string line;
            std::string error;
            
            int orderVertices=-1;
            int orderIndices=-1;
            
            int vertexCoordsFound=0;
            int colorCompsFound=0;
            int texCoordsFound=0;
            int normalsCoordsFound=0;
            
            int currentVertex = 0;
            int currentFace = 0;
            
            bool floatColor = false;
            
            enum State{
                Header,
                VertexDef,
                FaceDef,
                Vertices,
                Normals,
                Faces
            };
            
            State state = Header;
            
            int lineNum = 0;
            
            std::vector<glm::vec4> colors;
            std::vector<glm::vec3> vertices;
            std::vector<glm::vec3> normals;
            std::vector<glm::vec2> texcoord;
            std::vector<uint16_t> indices;

            std::getline(is,line);
            lineNum++;
            if(line!="ply"){
                error = "wrong format, expecting 'ply'";
                goto clean;
            }
            
            std::getline(is,line);
            lineNum++;
            if(line!="format ascii 1.0"){
                error = "wrong format, expecting 'format ascii 1.0'";
                goto clean;
            }
            
            while(std::getline(is,line)){
                lineNum++;
                if(line.find("comment")==0){
                    continue;
                }
                
                if((state==Header || state==FaceDef) && line.find("element vertex")==0){
                    state = VertexDef;
                    orderVertices = MAX(orderIndices, 0)+1;
                    vertices.resize(toInt(line.substr(15)));
                    continue;
                }
                
                if((state==Header || state==VertexDef) && line.find("element face")==0){
                    state = FaceDef;
                    orderIndices = MAX(orderVertices, 0)+1;
                    indices.resize(toInt(line.substr(13))*3);
                    continue;
                }
                
                if(state==VertexDef && (line.find("property float x")==0 || line.find("property float y")==0 || line.find("property float z")==0)){
                    vertexCoordsFound++;
                    continue;
                }
                
                if(state==VertexDef && (line.find("property float r")==0 || line.find("property float g")==0 || line.find("property float b")==0 || line.find("property float a")==0)){
                    colorCompsFound++;
                    colors.resize(vertices.size());
                    floatColor = true;
                    continue;
                }
                
                if(state==VertexDef && (line.find("property uchar red")==0 || line.find("property uchar green")==0 || line.find("property uchar blue")==0 || line.find("property uchar alpha")==0)){
                    colorCompsFound++;
                    colors.resize(vertices.size());
                    floatColor = false;
                    continue;
                }
                
                if(state==VertexDef && (line.find("property float u")==0 || line.find("property float v")==0)){
                    texCoordsFound++;
                    texcoord.resize(vertices.size());
                    continue;
                }
                
                if(state==VertexDef && (line.find("property float nx")==0 || line.find("property float ny")==0 || line.find("property float nz")==0)){
                    normalsCoordsFound++;
                    if (normalsCoordsFound==3) normals.resize(vertices.size());
                    continue;
                }
                
                if(state==FaceDef && line.find("property list")!=0 && line!="end_header"){
                    error = "wrong face definition";
                    goto clean;
                }
                
                if(line=="end_header"){
                    if(colors.size() && colorCompsFound!=3 && colorCompsFound!=4){
                        error =  "data has color coordiantes but not correct number of components. Found " + toString(colorCompsFound) + " expecting 3 or 4";
                        goto clean;
                    }
                    if(normals.size() && normalsCoordsFound!=3){
                        error = "data has normal coordiantes but not correct number of components. Found " + toString(normalsCoordsFound) + " expecting 3";
                        goto clean;
                    }
                    if(!vertices.size()){
                        std::cout << "ERROR glMesh, load(): mesh loaded from \"" << _file << "\" has no vertices" << std::endl;
                    }
                    if(orderVertices==-1) orderVertices=9999;
                    if(orderIndices==-1) orderIndices=9999;
                    
                    if(orderVertices < orderIndices){
                        state = Vertices;
                    }else {
                        state = Faces;
                    }
                    continue;
                }
                
                if(state==Vertices){
                    std::stringstream sline;
                    sline.str(line);
                    glm::vec3 v;
                    sline >> v.x;
                    sline >> v.y;
                    if(vertexCoordsFound>2) sline >> v.z;
                    vertices[currentVertex] = v;
                    
                    if(colorCompsFound>0){
                        if (floatColor){
                            glm::vec4 c;
                            sline >> c.r;
                            sline >> c.g;
                            sline >> c.b;
                            if(colorCompsFound>3) sline >> c.a;
                            colors[currentVertex] = c;
                        }else{
                            float r, g, b, a = 255;
                            sline >> r;
                            sline >> g;
                            sline >> b;
                            if(colorCompsFound>3) sline >> a;
                            colors[currentVertex] = glm::vec4(r/255.0, g/255.0, b/255.0, a/255.0);
                        }
                    }
                    
                    if(texCoordsFound>0){
                        glm::vec2 uv;
                        sline >> uv.x;
                        sline >> uv.y;
                        texcoord[currentVertex] = uv;
                    }
                    
                    if (normalsCoordsFound>0){
                        glm::vec3 n;
                        sline >> n.x;
                        sline >> n.y;
                        sline >> n.z;
                        normals[currentVertex] = n;
                    }
                    
                    currentVertex++;
                    if(currentVertex==vertices.size()){
                        if(orderVertices<orderIndices){
                            state = Faces;
                        }else{
                            state = Vertices;
                        }
                    }
                    continue;
                }
                
                if(state==Faces){
                    std::stringstream sline;
                    sline.str(line);
                    int numV;
                    sline >> numV;
                    if(numV!=3){
                        error = "face not a triangle";
                        goto clean;
                    }
                    int i;
                    sline >> i;
                    indices[currentFace*3] = i;
                    sline >> i;
                    indices[currentFace*3+1] = i;
                    sline >> i;
                    indices[currentFace*3+2] = i;
                    
                    currentFace++;
                    if(currentFace==indices.size()/3){
                        if(orderVertices<orderIndices){
                            state = Vertices;
                        }else{
                            state = Faces;
                        }
                    }
                    continue;
                }
            }
            is.close();
            
            //  Succed loading the PLY data
            //  (proceed replacing the data on mesh)
            //
            clear();
            addColors(colors);
            addVertices(vertices);
            addTexCoords(texcoord);
            addIndices(indices);
            
            if(normals.size()>0 && ( getDrawMode() == GL_TRIANGLES || getDrawMode() == GL_TRIANGLE_STRIP)){
                addNormals(normals);
            } else {
                computeNormals();
            }
            
            return true;
            
        clean:
            std::cout << "ERROR glMesh, load(): " << lineNum << ":" << error << std::endl;
            std::cout << "ERROR glMesh, load(): \"" << line << "\"" << std::endl;
        
        }
        
        is.close();
        std::cout << "ERROR glMesh, can not load  " << _file << std::endl;
        return false;
    }
}

void Mesh::setDrawMode(GLenum _drawMode) {
    m_drawMode = _drawMode;
}

void Mesh::setColor(const glm::vec4 &_color) {
    m_colors.clear();
    for (int i = 0; i < m_vertices.size(); i++) {
        m_colors.push_back(_color);
    }
}

void Mesh::addColor(const glm::vec4 &_color) {
    m_colors.push_back(_color);
}

void Mesh::addColors(const std::vector<glm::vec4> &_colors) {
    m_colors.insert(m_colors.end(), _colors.begin(), _colors.end());
}

void Mesh::addVertex(const glm::vec3 &_point){
   m_vertices.push_back(_point);
}

void Mesh::addVertices(const std::vector<glm::vec3>& _verts){
   m_vertices.insert(m_vertices.end(),_verts.begin(),_verts.end());
}

void Mesh::addVertices(const glm::vec3* verts, int amt){
   m_vertices.insert(m_vertices.end(),verts,verts+amt);
}

void Mesh::addNormal(const glm::vec3 &_normal){
    m_normals.push_back(_normal);
}

void Mesh::addNormals(const std::vector<glm::vec3> &_normals ){
    m_normals.insert(m_normals.end(), _normals.begin(), _normals.end());
}

void Mesh::addTexCoord(const glm::vec2 &_uv){
    m_texCoords.push_back(_uv);
}

void Mesh::addTexCoords(const std::vector<glm::vec2> &_uvs){
    m_texCoords.insert(m_texCoords.end(), _uvs.begin(), _uvs.end());
}

void Mesh::addIndex(uint16_t _i){
    m_indices.push_back(_i);
}

void Mesh::addIndices(const std::vector<uint16_t>& inds){
	m_indices.insert(m_indices.end(),inds.begin(),inds.end());
}

void Mesh::addIndices(const uint16_t* inds, int amt){
	m_indices.insert(m_indices.end(),inds,inds+amt);
}

void Mesh::addTriangle(uint16_t index1, uint16_t index2, uint16_t index3){
    addIndex(index1);
    addIndex(index2);
    addIndex(index3);
}

void Mesh::add(const Mesh &_mesh){
    
    if(_mesh.getDrawMode() != m_drawMode){
        std::cout << "INCOMPATIBLE DRAW MODES" << std::endl;
        return;
    }
    
    uint16_t indexOffset = (uint16_t)getVertices().size();
    
    addColors(_mesh.getColors());
    addVertices(_mesh.getVertices());
    addNormals(_mesh.getNormals());
    addTexCoords(_mesh.getTexCoords());
    
    for (int i = 0; i < _mesh.getIndices().size(); i++) {
        addIndex(indexOffset+_mesh.getIndices()[i]);
    }
}

GLenum Mesh::getDrawMode() const{
    return m_drawMode;
}

const std::vector<glm::vec4> & Mesh::getColors() const{
    return m_colors;
}

const std::vector<glm::vec3> & Mesh::getVertices() const{
	return m_vertices;
}

const std::vector<glm::vec3> & Mesh::getNormals() const{
    return m_normals;
}

const std::vector<glm::vec2> & Mesh::getTexCoords() const{
    return m_texCoords;
}

const std::vector<glm::uint16_t> & Mesh::getIndices() const{
    return m_indices;
}

std::vector<glm::ivec3> Mesh::getTriangles() const {
    std::vector<glm::ivec3> faces;
    
    if(getDrawMode() == GL_TRIANGLES) {
        if(m_indices.size()>0){
            for(unsigned int j = 0; j < m_indices.size(); j += 3) {
                glm::ivec3 tri;
                for(int k = 0; k < 3; k++) {
                    tri[k] = m_indices[j+k];
                }
                faces.push_back(tri);
            }
        } else {
            for( unsigned int j = 0; j < m_vertices.size(); j += 3) {
                glm::ivec3 tri;
                for(int k = 0; k < 3; k++) {
                    tri[k] = j+k;
                }
                faces.push_back(tri);
            }
        }
    } else {
        //  TODO
        //
        std::cout << "ERROR: Mesh only add GL_TRIANGLES for NOW !!" << std::endl;
    }
    
    return faces;
}

void Mesh::clear(){
    if(!m_vertices.empty()){
		m_vertices.clear();
	}
	if(!m_colors.empty()){
		m_colors.clear();
	}
	if(!m_normals.empty()){
		m_normals.clear();
	}
    if(!m_indices.empty()){
		m_indices.clear();
	}
}

void Mesh::computeNormals(){
    
    if(getDrawMode() == GL_TRIANGLES){
        //The number of the vertices
        int nV = m_vertices.size();
        
        //The number of the triangles
        int nT = m_indices.size() / 3;
        
        std::vector<glm::vec3> norm( nV ); //Array for the normals
        
        //Scan all the triangles. For each triangle add its
        //normal to norm's vectors of triangle's vertices
        for (int t=0; t<nT; t++) {
            
            //Get indices of the triangle t
            int i1 = m_indices[ 3 * t ];
            int i2 = m_indices[ 3 * t + 1 ];
            int i3 = m_indices[ 3 * t + 2 ];
            
            //Get vertices of the triangle
            const glm::vec3 &v1 = m_vertices[ i1 ];
            const glm::vec3 &v2 = m_vertices[ i2 ];
            const glm::vec3 &v3 = m_vertices[ i3 ];
            
            //Compute the triangle's normal
            glm::vec3 dir = glm::normalize(glm::cross(v2-v1,v3-v1));
            
            //Accumulate it to norm array for i1, i2, i3
            norm[ i1 ] += dir;
            norm[ i2 ] += dir;
            norm[ i3 ] += dir;
        }
        
        //Normalize the normal's length and add it.
        m_normals.clear();
        for (int i=0; i<nV; i++) {
            addNormal( glm::normalize(norm[i]) );
        }
        
    } else {
        //  TODO
        //
        std::cout << "ERROR: Mesh only add GL_TRIANGLES for NOW !!" << std::endl;
    }
}

Vbo* Mesh::getVbo(){

    // Create Vertex Layout
    //
    std::vector<VertexLayout::VertexAttrib> attribs;
    attribs.push_back({"a_position", 3, GL_FLOAT, false, 0});
    bool bColor = false;
    bool bNormals = false;
    bool bTexCoords = false;
    int  nBits = 3; 

    if (m_colors.size() > 0 && m_colors.size() == m_vertices.size()){
        attribs.push_back({"a_color", 4, GL_FLOAT, false, 0});
        bColor = true;
        nBits += 4;
    }
    if (m_normals.size() > 0 && m_normals.size() == m_vertices.size()){
        attribs.push_back({"a_normal", 3, GL_FLOAT, false, 0});
        bNormals = true;
        nBits += 3;
    }
    if (m_texCoords.size() > 0 && m_texCoords.size() == m_vertices.size()){
        attribs.push_back({"a_texcoord", 2, GL_FLOAT, false, 0});
        bTexCoords = true;
        nBits += 2;
    }

    VertexLayout* vertexLayout = new VertexLayout(attribs);
    Vbo* tmpMesh = new Vbo(vertexLayout);

    // Copy data
    std::vector<GLfloat> data;
    for(int i = 0; i < m_vertices.size(); i++){ 
        data.push_back(m_vertices[i].x);
        data.push_back(m_vertices[i].y);
        data.push_back(m_vertices[i].z);
        if(bColor){
            data.push_back(m_colors[i].r);
            data.push_back(m_colors[i].g);
            data.push_back(m_colors[i].b);
            data.push_back(m_colors[i].a);
        }
        if(bNormals){
            data.push_back(m_normals[i].x);
            data.push_back(m_normals[i].y);
            data.push_back(m_normals[i].z);
        }
        if(bTexCoords){
            data.push_back(m_texCoords[i].x);
            data.push_back(m_texCoords[i].y);
        }
    }

    tmpMesh->addVertices((GLbyte*)data.data(), data.size());
    tmpMesh->addIndices(m_indices.data(), m_indices.size());

    if(m_indices.size()>0){
        tmpMesh->addIndices(m_indices.data(), m_indices.size());
    } else {
        tmpMesh->setDrawMode(getDrawMode());
    }

    return tmpMesh;
}