//#ifndef KAWA_DE_SERIALIZE
//#define KAWA_DE_SERIALIZE
//
//#include <fstream>
//#include "core_types.h"
//#include "meta.h"
//
//namespace kawa
//{
//	template<typename T>
//	struct serialize_trait
//	{
//		static void write(std::ofstream& out, T& value)
//		{
//			out << value << '\n';
//		}
//	};
//
//
//	struct serializer_map
//	{
//		using fn_t = void(std::ofstream&, void*);
//		template<typename T>
//		static inline fn_t* fn = +[](std::ofstream& out, void* ptr)
//			{
//				serialize_trait<T>::write(out, *static_cast<T*>(ptr));
//			};
//
//		template<typename T>
//		void ensure()
//		{
//			serializers[type_hash<T>] = fn<T>;
//		}
//
//		umap<u64, fn_t*> serializers;
//	};
//
//	struct serializer
//	{
//		std::ofstream file{ "test.txt" };
//
//		template<typename T>
//		void put(T& value)
//		{
//			file << type_hash<T> << '\n';
//			serialize_trait<T>::write(file, value);
//		}
//
//		void put(serializer_map& map, u64 type_hash, void* value)
//		{
//			file << type_hash << '\n';
//			map.serializers[type_hash](file, value);
//		}
//	};
//
//
//	template<typename T>
//	struct deserialize_trait
//	{
//		static void read(std::ifstream& in, T& value)
//		{
//			in >> value;
//		}
//	};
//
//
//	struct deserializer_map
//	{
//		using fn_t = void(std::ifstream&, void*);
//		template<typename T>
//		static inline fn_t* fn = +[](std::ifstream& in, void* ptr)
//			{
//				deserialize_trait<T>::read(in, *static_cast<T*>(ptr));
//			};
//
//		template<typename T>
//		void ensure()
//		{
//			deserializers[type_hash<T>] = fn<T>;
//		}
//
//		umap<u64, fn_t*> deserializers;
//	};
//
//	struct deserializer
//	{
//		std::ifstream file{ "test.txt" };
//
//		template<typename T>
//		void get(T& value)
//		{
//			file << type_hash<T> << '\n';
//			deserialize_trait<T>::read(file, value);
//		}
//
//		void get(deserializer_map& map, void* value)
//		{
//			u64 type_hash;
//			file >> type_hash;
//			map.deserializers[type_hash](file, value);
//		}
//	};
//}
//#endif // !KAWA_DE_SERIALIZE
