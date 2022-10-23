// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: material.proto

#ifndef GOOGLE_PROTOBUF_INCLUDED_material_2eproto
#define GOOGLE_PROTOBUF_INCLUDED_material_2eproto

#include <limits>
#include <string>

#include <google/protobuf/port_def.inc>
#if PROTOBUF_VERSION < 3021000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers. Please update
#error your headers.
#endif
#if 3021008 < PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers. Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/port_undef.inc>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/arena.h>
#include <google/protobuf/arenastring.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/metadata_lite.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>  // IWYU pragma: export
#include <google/protobuf/extension_set.h>  // IWYU pragma: export
#include <google/protobuf/unknown_field_set.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>
#define PROTOBUF_INTERNAL_EXPORT_material_2eproto
PROTOBUF_NAMESPACE_OPEN
namespace internal {
class AnyMetadata;
}  // namespace internal
PROTOBUF_NAMESPACE_CLOSE

// Internal implementation detail -- do not use these members.
struct TableStruct_material_2eproto {
  static const uint32_t offsets[];
};
extern const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable descriptor_table_material_2eproto;
namespace frame {
namespace proto {
class Material;
struct MaterialDefaultTypeInternal;
extern MaterialDefaultTypeInternal _Material_default_instance_;
}  // namespace proto
}  // namespace frame
PROTOBUF_NAMESPACE_OPEN
template<> ::frame::proto::Material* Arena::CreateMaybeMessage<::frame::proto::Material>(Arena*);
PROTOBUF_NAMESPACE_CLOSE
namespace frame {
namespace proto {

// ===================================================================

class Material final :
    public ::PROTOBUF_NAMESPACE_ID::Message /* @@protoc_insertion_point(class_definition:frame.proto.Material) */ {
 public:
  inline Material() : Material(nullptr) {}
  ~Material() override;
  explicit PROTOBUF_CONSTEXPR Material(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized);

  Material(const Material& from);
  Material(Material&& from) noexcept
    : Material() {
    *this = ::std::move(from);
  }

  inline Material& operator=(const Material& from) {
    CopyFrom(from);
    return *this;
  }
  inline Material& operator=(Material&& from) noexcept {
    if (this == &from) return *this;
    if (GetOwningArena() == from.GetOwningArena()
  #ifdef PROTOBUF_FORCE_COPY_IN_MOVE
        && GetOwningArena() != nullptr
  #endif  // !PROTOBUF_FORCE_COPY_IN_MOVE
    ) {
      InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }

  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* descriptor() {
    return GetDescriptor();
  }
  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* GetDescriptor() {
    return default_instance().GetMetadata().descriptor;
  }
  static const ::PROTOBUF_NAMESPACE_ID::Reflection* GetReflection() {
    return default_instance().GetMetadata().reflection;
  }
  static const Material& default_instance() {
    return *internal_default_instance();
  }
  static inline const Material* internal_default_instance() {
    return reinterpret_cast<const Material*>(
               &_Material_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    0;

  friend void swap(Material& a, Material& b) {
    a.Swap(&b);
  }
  inline void Swap(Material* other) {
    if (other == this) return;
  #ifdef PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetOwningArena() != nullptr &&
        GetOwningArena() == other->GetOwningArena()) {
   #else  // PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetOwningArena() == other->GetOwningArena()) {
  #endif  // !PROTOBUF_FORCE_COPY_IN_SWAP
      InternalSwap(other);
    } else {
      ::PROTOBUF_NAMESPACE_ID::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(Material* other) {
    if (other == this) return;
    GOOGLE_DCHECK(GetOwningArena() == other->GetOwningArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  Material* New(::PROTOBUF_NAMESPACE_ID::Arena* arena = nullptr) const final {
    return CreateMaybeMessage<Material>(arena);
  }
  using ::PROTOBUF_NAMESPACE_ID::Message::CopyFrom;
  void CopyFrom(const Material& from);
  using ::PROTOBUF_NAMESPACE_ID::Message::MergeFrom;
  void MergeFrom( const Material& from) {
    Material::MergeImpl(*this, from);
  }
  private:
  static void MergeImpl(::PROTOBUF_NAMESPACE_ID::Message& to_msg, const ::PROTOBUF_NAMESPACE_ID::Message& from_msg);
  public:
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  const char* _InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) final;
  uint8_t* _InternalSerialize(
      uint8_t* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const final;
  int GetCachedSize() const final { return _impl_._cached_size_.Get(); }

  private:
  void SharedCtor(::PROTOBUF_NAMESPACE_ID::Arena* arena, bool is_message_owned);
  void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(Material* other);

  private:
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::PROTOBUF_NAMESPACE_ID::StringPiece FullMessageName() {
    return "frame.proto.Material";
  }
  protected:
  explicit Material(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                       bool is_message_owned = false);
  public:

  static const ClassData _class_data_;
  const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*GetClassData() const final;

  ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  enum : int {
    kTextureNamesFieldNumber = 3,
    kInnerNamesFieldNumber = 4,
    kNameFieldNumber = 1,
    kProgramNameFieldNumber = 5,
  };
  // repeated string texture_names = 3;
  int texture_names_size() const;
  private:
  int _internal_texture_names_size() const;
  public:
  void clear_texture_names();
  const std::string& texture_names(int index) const;
  std::string* mutable_texture_names(int index);
  void set_texture_names(int index, const std::string& value);
  void set_texture_names(int index, std::string&& value);
  void set_texture_names(int index, const char* value);
  void set_texture_names(int index, const char* value, size_t size);
  std::string* add_texture_names();
  void add_texture_names(const std::string& value);
  void add_texture_names(std::string&& value);
  void add_texture_names(const char* value);
  void add_texture_names(const char* value, size_t size);
  const ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField<std::string>& texture_names() const;
  ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField<std::string>* mutable_texture_names();
  private:
  const std::string& _internal_texture_names(int index) const;
  std::string* _internal_add_texture_names();
  public:

  // repeated string inner_names = 4;
  int inner_names_size() const;
  private:
  int _internal_inner_names_size() const;
  public:
  void clear_inner_names();
  const std::string& inner_names(int index) const;
  std::string* mutable_inner_names(int index);
  void set_inner_names(int index, const std::string& value);
  void set_inner_names(int index, std::string&& value);
  void set_inner_names(int index, const char* value);
  void set_inner_names(int index, const char* value, size_t size);
  std::string* add_inner_names();
  void add_inner_names(const std::string& value);
  void add_inner_names(std::string&& value);
  void add_inner_names(const char* value);
  void add_inner_names(const char* value, size_t size);
  const ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField<std::string>& inner_names() const;
  ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField<std::string>* mutable_inner_names();
  private:
  const std::string& _internal_inner_names(int index) const;
  std::string* _internal_add_inner_names();
  public:

  // string name = 1;
  void clear_name();
  const std::string& name() const;
  template <typename ArgT0 = const std::string&, typename... ArgT>
  void set_name(ArgT0&& arg0, ArgT... args);
  std::string* mutable_name();
  PROTOBUF_NODISCARD std::string* release_name();
  void set_allocated_name(std::string* name);
  private:
  const std::string& _internal_name() const;
  inline PROTOBUF_ALWAYS_INLINE void _internal_set_name(const std::string& value);
  std::string* _internal_mutable_name();
  public:

  // string program_name = 5;
  void clear_program_name();
  const std::string& program_name() const;
  template <typename ArgT0 = const std::string&, typename... ArgT>
  void set_program_name(ArgT0&& arg0, ArgT... args);
  std::string* mutable_program_name();
  PROTOBUF_NODISCARD std::string* release_program_name();
  void set_allocated_program_name(std::string* program_name);
  private:
  const std::string& _internal_program_name() const;
  inline PROTOBUF_ALWAYS_INLINE void _internal_set_program_name(const std::string& value);
  std::string* _internal_mutable_program_name();
  public:

  // @@protoc_insertion_point(class_scope:frame.proto.Material)
 private:
  class _Internal;

  template <typename T> friend class ::PROTOBUF_NAMESPACE_ID::Arena::InternalHelper;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  struct Impl_ {
    ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField<std::string> texture_names_;
    ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField<std::string> inner_names_;
    ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr name_;
    ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr program_name_;
    mutable ::PROTOBUF_NAMESPACE_ID::internal::CachedSize _cached_size_;
  };
  union { Impl_ _impl_; };
  friend struct ::TableStruct_material_2eproto;
};
// ===================================================================


// ===================================================================

#ifdef __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif  // __GNUC__
// Material

// string name = 1;
inline void Material::clear_name() {
  _impl_.name_.ClearToEmpty();
}
inline const std::string& Material::name() const {
  // @@protoc_insertion_point(field_get:frame.proto.Material.name)
  return _internal_name();
}
template <typename ArgT0, typename... ArgT>
inline PROTOBUF_ALWAYS_INLINE
void Material::set_name(ArgT0&& arg0, ArgT... args) {
 
 _impl_.name_.Set(static_cast<ArgT0 &&>(arg0), args..., GetArenaForAllocation());
  // @@protoc_insertion_point(field_set:frame.proto.Material.name)
}
inline std::string* Material::mutable_name() {
  std::string* _s = _internal_mutable_name();
  // @@protoc_insertion_point(field_mutable:frame.proto.Material.name)
  return _s;
}
inline const std::string& Material::_internal_name() const {
  return _impl_.name_.Get();
}
inline void Material::_internal_set_name(const std::string& value) {
  
  _impl_.name_.Set(value, GetArenaForAllocation());
}
inline std::string* Material::_internal_mutable_name() {
  
  return _impl_.name_.Mutable(GetArenaForAllocation());
}
inline std::string* Material::release_name() {
  // @@protoc_insertion_point(field_release:frame.proto.Material.name)
  return _impl_.name_.Release();
}
inline void Material::set_allocated_name(std::string* name) {
  if (name != nullptr) {
    
  } else {
    
  }
  _impl_.name_.SetAllocated(name, GetArenaForAllocation());
#ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (_impl_.name_.IsDefault()) {
    _impl_.name_.Set("", GetArenaForAllocation());
  }
#endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  // @@protoc_insertion_point(field_set_allocated:frame.proto.Material.name)
}

// string program_name = 5;
inline void Material::clear_program_name() {
  _impl_.program_name_.ClearToEmpty();
}
inline const std::string& Material::program_name() const {
  // @@protoc_insertion_point(field_get:frame.proto.Material.program_name)
  return _internal_program_name();
}
template <typename ArgT0, typename... ArgT>
inline PROTOBUF_ALWAYS_INLINE
void Material::set_program_name(ArgT0&& arg0, ArgT... args) {
 
 _impl_.program_name_.Set(static_cast<ArgT0 &&>(arg0), args..., GetArenaForAllocation());
  // @@protoc_insertion_point(field_set:frame.proto.Material.program_name)
}
inline std::string* Material::mutable_program_name() {
  std::string* _s = _internal_mutable_program_name();
  // @@protoc_insertion_point(field_mutable:frame.proto.Material.program_name)
  return _s;
}
inline const std::string& Material::_internal_program_name() const {
  return _impl_.program_name_.Get();
}
inline void Material::_internal_set_program_name(const std::string& value) {
  
  _impl_.program_name_.Set(value, GetArenaForAllocation());
}
inline std::string* Material::_internal_mutable_program_name() {
  
  return _impl_.program_name_.Mutable(GetArenaForAllocation());
}
inline std::string* Material::release_program_name() {
  // @@protoc_insertion_point(field_release:frame.proto.Material.program_name)
  return _impl_.program_name_.Release();
}
inline void Material::set_allocated_program_name(std::string* program_name) {
  if (program_name != nullptr) {
    
  } else {
    
  }
  _impl_.program_name_.SetAllocated(program_name, GetArenaForAllocation());
#ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (_impl_.program_name_.IsDefault()) {
    _impl_.program_name_.Set("", GetArenaForAllocation());
  }
#endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  // @@protoc_insertion_point(field_set_allocated:frame.proto.Material.program_name)
}

// repeated string texture_names = 3;
inline int Material::_internal_texture_names_size() const {
  return _impl_.texture_names_.size();
}
inline int Material::texture_names_size() const {
  return _internal_texture_names_size();
}
inline void Material::clear_texture_names() {
  _impl_.texture_names_.Clear();
}
inline std::string* Material::add_texture_names() {
  std::string* _s = _internal_add_texture_names();
  // @@protoc_insertion_point(field_add_mutable:frame.proto.Material.texture_names)
  return _s;
}
inline const std::string& Material::_internal_texture_names(int index) const {
  return _impl_.texture_names_.Get(index);
}
inline const std::string& Material::texture_names(int index) const {
  // @@protoc_insertion_point(field_get:frame.proto.Material.texture_names)
  return _internal_texture_names(index);
}
inline std::string* Material::mutable_texture_names(int index) {
  // @@protoc_insertion_point(field_mutable:frame.proto.Material.texture_names)
  return _impl_.texture_names_.Mutable(index);
}
inline void Material::set_texture_names(int index, const std::string& value) {
  _impl_.texture_names_.Mutable(index)->assign(value);
  // @@protoc_insertion_point(field_set:frame.proto.Material.texture_names)
}
inline void Material::set_texture_names(int index, std::string&& value) {
  _impl_.texture_names_.Mutable(index)->assign(std::move(value));
  // @@protoc_insertion_point(field_set:frame.proto.Material.texture_names)
}
inline void Material::set_texture_names(int index, const char* value) {
  GOOGLE_DCHECK(value != nullptr);
  _impl_.texture_names_.Mutable(index)->assign(value);
  // @@protoc_insertion_point(field_set_char:frame.proto.Material.texture_names)
}
inline void Material::set_texture_names(int index, const char* value, size_t size) {
  _impl_.texture_names_.Mutable(index)->assign(
    reinterpret_cast<const char*>(value), size);
  // @@protoc_insertion_point(field_set_pointer:frame.proto.Material.texture_names)
}
inline std::string* Material::_internal_add_texture_names() {
  return _impl_.texture_names_.Add();
}
inline void Material::add_texture_names(const std::string& value) {
  _impl_.texture_names_.Add()->assign(value);
  // @@protoc_insertion_point(field_add:frame.proto.Material.texture_names)
}
inline void Material::add_texture_names(std::string&& value) {
  _impl_.texture_names_.Add(std::move(value));
  // @@protoc_insertion_point(field_add:frame.proto.Material.texture_names)
}
inline void Material::add_texture_names(const char* value) {
  GOOGLE_DCHECK(value != nullptr);
  _impl_.texture_names_.Add()->assign(value);
  // @@protoc_insertion_point(field_add_char:frame.proto.Material.texture_names)
}
inline void Material::add_texture_names(const char* value, size_t size) {
  _impl_.texture_names_.Add()->assign(reinterpret_cast<const char*>(value), size);
  // @@protoc_insertion_point(field_add_pointer:frame.proto.Material.texture_names)
}
inline const ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField<std::string>&
Material::texture_names() const {
  // @@protoc_insertion_point(field_list:frame.proto.Material.texture_names)
  return _impl_.texture_names_;
}
inline ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField<std::string>*
Material::mutable_texture_names() {
  // @@protoc_insertion_point(field_mutable_list:frame.proto.Material.texture_names)
  return &_impl_.texture_names_;
}

// repeated string inner_names = 4;
inline int Material::_internal_inner_names_size() const {
  return _impl_.inner_names_.size();
}
inline int Material::inner_names_size() const {
  return _internal_inner_names_size();
}
inline void Material::clear_inner_names() {
  _impl_.inner_names_.Clear();
}
inline std::string* Material::add_inner_names() {
  std::string* _s = _internal_add_inner_names();
  // @@protoc_insertion_point(field_add_mutable:frame.proto.Material.inner_names)
  return _s;
}
inline const std::string& Material::_internal_inner_names(int index) const {
  return _impl_.inner_names_.Get(index);
}
inline const std::string& Material::inner_names(int index) const {
  // @@protoc_insertion_point(field_get:frame.proto.Material.inner_names)
  return _internal_inner_names(index);
}
inline std::string* Material::mutable_inner_names(int index) {
  // @@protoc_insertion_point(field_mutable:frame.proto.Material.inner_names)
  return _impl_.inner_names_.Mutable(index);
}
inline void Material::set_inner_names(int index, const std::string& value) {
  _impl_.inner_names_.Mutable(index)->assign(value);
  // @@protoc_insertion_point(field_set:frame.proto.Material.inner_names)
}
inline void Material::set_inner_names(int index, std::string&& value) {
  _impl_.inner_names_.Mutable(index)->assign(std::move(value));
  // @@protoc_insertion_point(field_set:frame.proto.Material.inner_names)
}
inline void Material::set_inner_names(int index, const char* value) {
  GOOGLE_DCHECK(value != nullptr);
  _impl_.inner_names_.Mutable(index)->assign(value);
  // @@protoc_insertion_point(field_set_char:frame.proto.Material.inner_names)
}
inline void Material::set_inner_names(int index, const char* value, size_t size) {
  _impl_.inner_names_.Mutable(index)->assign(
    reinterpret_cast<const char*>(value), size);
  // @@protoc_insertion_point(field_set_pointer:frame.proto.Material.inner_names)
}
inline std::string* Material::_internal_add_inner_names() {
  return _impl_.inner_names_.Add();
}
inline void Material::add_inner_names(const std::string& value) {
  _impl_.inner_names_.Add()->assign(value);
  // @@protoc_insertion_point(field_add:frame.proto.Material.inner_names)
}
inline void Material::add_inner_names(std::string&& value) {
  _impl_.inner_names_.Add(std::move(value));
  // @@protoc_insertion_point(field_add:frame.proto.Material.inner_names)
}
inline void Material::add_inner_names(const char* value) {
  GOOGLE_DCHECK(value != nullptr);
  _impl_.inner_names_.Add()->assign(value);
  // @@protoc_insertion_point(field_add_char:frame.proto.Material.inner_names)
}
inline void Material::add_inner_names(const char* value, size_t size) {
  _impl_.inner_names_.Add()->assign(reinterpret_cast<const char*>(value), size);
  // @@protoc_insertion_point(field_add_pointer:frame.proto.Material.inner_names)
}
inline const ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField<std::string>&
Material::inner_names() const {
  // @@protoc_insertion_point(field_list:frame.proto.Material.inner_names)
  return _impl_.inner_names_;
}
inline ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField<std::string>*
Material::mutable_inner_names() {
  // @@protoc_insertion_point(field_mutable_list:frame.proto.Material.inner_names)
  return &_impl_.inner_names_;
}

#ifdef __GNUC__
  #pragma GCC diagnostic pop
#endif  // __GNUC__

// @@protoc_insertion_point(namespace_scope)

}  // namespace proto
}  // namespace frame

// @@protoc_insertion_point(global_scope)

#include <google/protobuf/port_undef.inc>
#endif  // GOOGLE_PROTOBUF_INCLUDED_GOOGLE_PROTOBUF_INCLUDED_material_2eproto