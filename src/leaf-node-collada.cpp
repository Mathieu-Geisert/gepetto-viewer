//
//  leaf-node-collada.cpp
//  gepetto-viewer
//
//  Created by Anthony Couret, Mathieu Geisert in November 2014.
//  Copyright (c) 2014 LAAS-CNRS. All rights reserved.
//

#include <sys/stat.h>
#include <fstream>
#include <clocale>
#include <ios>
#include <osgDB/FileNameUtils>
#include <gepetto/viewer/leaf-node-collada.h>

namespace graphics {
  inline bool fileExists (const char* fn)
  { struct stat buffer; return (stat (fn, &buffer) == 0); }

  std::string getCachedFileName (const std::string& meshfile)
  {
    static const std::string exts[3] = { ".osgb", ".osg2", ".osg" };
    for (int i = 0; i < 3; ++i) {
      std::string cached = meshfile + exts[i];
      if (fileExists(cached.c_str())) return cached;
    }
    return std::string();
  }
    
  /* Declaration of private function members */

  void LeafNodeCollada::init()
  {
    if (!fileExists(collada_file_path_.c_str()))
      throw std::invalid_argument(std::string("File ") + collada_file_path_ + std::string(" not found."));

    std::string osgname = getCachedFileName(collada_file_path_);
    if (!osgname.empty()) {
      std::cout << "Using " << osgname << std::endl;
      collada_ptr_ = osgDB::readNodeFile(osgname);
    } else {
      // get the extension of the meshs file
      std::string ext = osgDB::getLowerCaseFileExtension(collada_file_path_);
      if (ext == "dae" && *localeconv()->decimal_point != '.') {
        std::cerr << "Warning: your locale convention uses '"
          << localeconv()->decimal_point << "' as decimal separator while DAE "
          "expects '.'.\nSet LC_NUMERIC to a locale convetion using '.' as "
          "decimal separator (e.g. export LC_NUMERIC=\"en_US.utf-8\")."
          << std::endl;
      }
      if(ext == "obj"){
        const osgDB::Options* options = new osgDB::Options("noRotation");
        collada_ptr_ = osgDB::readNodeFile(collada_file_path_,options);
      }
      else
        collada_ptr_ = osgDB::readNodeFile(collada_file_path_);
      if (ext == "dae") {
        bool error = false;
        if (!collada_ptr_) {
          std::cout << "File: " << collada_file_path_ << " could not be loaded\n";
          error = true;
        } else if (strncasecmp(collada_ptr_->getName().c_str(), "empty", 5) == 0) {
          std::cout << "File: " << collada_file_path_ << " could not be loaded:\n"
            << collada_ptr_->getName() << '\n';
          error = true;
        }
        if (error) {
          std::cout << "You may try to convert the file with the following command:\n"
            "osgconv " << collada_file_path_ << ' ' << collada_file_path_ << ".osgb" << std::endl;
        }
      }
    }
    if (!collada_ptr_)
      throw std::invalid_argument(std::string("File ") + collada_file_path_ + std::string(" found but could not be opened. Check that a plugin exist."));
        
    /* Create PositionAttitudeTransform */
    this->asQueue()->addChild(collada_ptr_);
        
    /* Allow transparency */
    if (collada_ptr_->getOrCreateStateSet())
      {
	collada_ptr_->getOrCreateStateSet()->setMode(GL_BLEND, ::osg::StateAttribute::ON);
      }

    addProperty(StringProperty::create("Meshfile path",
          StringProperty::getterFromMemberFunction(this, &LeafNodeCollada::meshFilePath),
          StringProperty::Setter_t()));
  }
    
  LeafNodeCollada::LeafNodeCollada(const std::string& name, const std::string& collada_file_path) :
    Node(name), collada_file_path_(collada_file_path)
  {
    init();
  }
    
  LeafNodeCollada::LeafNodeCollada(const std::string& name, const std::string& collada_file_path, const osgVector4& color) :
    Node(name), collada_file_path_(collada_file_path)
  {
    init();
    setColor(color);
  }
  
  LeafNodeCollada::LeafNodeCollada(const LeafNodeCollada& other) :
    Node(other.getID()), collada_file_path_(other.collada_file_path_)
  {
    init();
  }
    
  void LeafNodeCollada::initWeakPtr(LeafNodeColladaWeakPtr other_weak_ptr)
  {
    weak_ptr_ = other_weak_ptr;
  }
    
  /* End of declaration of private function members */
    
  /* Declaration of protected function members */
    
  LeafNodeColladaPtr_t LeafNodeCollada::create(const std::string& name, const std::string& collada_file_path)
  {
    std::ifstream infile(collada_file_path.c_str ());
    if (!infile.good()) {
      throw std::ios_base::failure (collada_file_path +
                                    std::string (" does not exist."));
    }
    LeafNodeColladaPtr_t shared_ptr(new LeafNodeCollada
                                    (name, collada_file_path));
    
    // Add reference to itself
    shared_ptr->initWeakPtr(shared_ptr);
    
    return shared_ptr;
  }

    LeafNodeColladaPtr_t LeafNodeCollada::create(const std::string& name, const std::string& collada_file_path, const osgVector4& color)
  {
    std::ifstream infile(collada_file_path.c_str ());
    if (!infile.good()) {
      throw std::ios_base::failure (collada_file_path +
                                    std::string (" does not exist."));
    }
    LeafNodeColladaPtr_t shared_ptr(new LeafNodeCollada
                                    (name, collada_file_path, color));
    
    // Add reference to itself
    shared_ptr->initWeakPtr(shared_ptr);
    
    return shared_ptr;
  }
    
  LeafNodeColladaPtr_t LeafNodeCollada::createCopy(LeafNodeColladaPtr_t other)
  {
    LeafNodeColladaPtr_t shared_ptr(new LeafNodeCollada(*other));
        
    // Add reference to itself
    shared_ptr->initWeakPtr(shared_ptr);
        
    return shared_ptr;
  }

  ::osg::NodeRefPtr LeafNodeCollada::getColladaPtr()
  {
    return collada_ptr_.get();
  }

  /* End of declaration of protected function members */
    
  /* Declaration of public function members */
    
  LeafNodeColladaPtr_t LeafNodeCollada::clone(void) const
  {
    return LeafNodeCollada::createCopy(weak_ptr_.lock());
  }
    
  LeafNodeColladaPtr_t LeafNodeCollada::self(void) const
  {
    return weak_ptr_.lock();
  }
    
  void LeafNodeCollada::setColor(const osgVector4& color)
  {
    osg::ref_ptr<osg::Material> mat_ptr (new osg::Material); 
    osgVector4 ambient (color.r() * 0.5f, color.g() * 0.5f, color.b() * 0.5f, color.a());

    mat_ptr->setDiffuse (osg::Material::FRONT_AND_BACK,color); 
    mat_ptr->setAmbient (osg::Material::FRONT_AND_BACK,ambient); 

    collada_ptr_->getOrCreateStateSet()->setAttribute(mat_ptr.get());    
  }

  void LeafNodeCollada::setAlpha(const float& alpha)
  {
    osg::StateSet* ss = getColladaPtr().get()->getStateSet();
    if (ss)
      {
	alpha_ = alpha;
	osg::Material *mat;
	if (ss->getAttribute(osg::StateAttribute::MATERIAL))
	  mat = dynamic_cast<osg::Material*>(ss->getAttribute(osg::StateAttribute::MATERIAL));
	else
	  {
	    mat = new osg::Material;
	    ss->setAttribute(mat, osg::StateAttribute::OFF | osg::StateAttribute::OVERRIDE);
	  }
	mat->setTransparency(osg::Material::FRONT_AND_BACK, alpha);
	if (alpha == 0)
	  ss->setRenderingHint(osg::StateSet::DEFAULT_BIN);
	else
	  ss->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
      }
  }
 
  void LeafNodeCollada::setTexture(const std::string& image_path)
  {
    texture_file_path_ = image_path;
    osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D;
    texture->setDataVariance(osg::Object::DYNAMIC); 
    osg::ref_ptr<osg::Image> image = osgDB::readImageFile(image_path);
    if (!image)
    {
      std::cout << " couldn't find texture, quiting." << std::endl;
      return;
    } 
    texture->setImage(image);
    collada_ptr_->getStateSet()->setTextureAttributeAndModes(0,texture,osg::StateAttribute::ON);
  }

  const std::string& LeafNodeCollada::meshFilePath () const
  {
    return collada_file_path_;
  }

  const std::string& LeafNodeCollada::textureFilePath () const
  {
    return texture_file_path_;
  }

  /*void LeafNodeCollada::setColor(osg::NodeRefPtr osgNode_ptr,const osgVector4& color)
  {
    osg::Vec4ArrayRefPtr colorArray = new osg::Vec4Array();
    colorArray->push_back(color);
    osg::GeodeRefPtr geode_ptr = osgNode_ptr->asGeode();
    if (geode_ptr) {
      for (unsigned int i = 0 ; i < geode_ptr->getNumDrawables() ; i++) {
      osg::GeometryRefPtr geom_ptr = geode_ptr->getDrawable(i)->asGeometry();
        if (geom_ptr) {
          geom_ptr->setColorArray(colorArray.get());
          geom_ptr->setColorBinding(osg::Geometry::BIND_OVERALL); 
        }
      }
    }
    else {
      osg::GroupRefPtr group_ptr = osgNode_ptr->asGroup();
      if (group_ptr) {
        for (unsigned int i = 0 ; i < group_ptr->getNumChildren() ; i++) {
          setColor(group_ptr->getChild(i),color);
        }
      }
    }
  }*/

  osg::ref_ptr<osg::Node> LeafNodeCollada::getOsgNode() const
  {
    return collada_ptr_;
  }

  LeafNodeCollada::~LeafNodeCollada()
  {
    /* Proper deletion of all tree scene */
    this->asQueue()->removeChild(collada_ptr_);
    collada_ptr_ = NULL;
        
    weak_ptr_.reset();
  }
    
  /* End of declaration of public function members */
    
} /* namespace graphics */
