#include "ade_doc.h"

#include "globalincs/pstypes.h"

namespace scripting {

ade_type_info::ade_type_info(std::initializer_list<ade_type_info> tuple_types)
	: _type(ade_type_info_type::Tuple), _elements(tuple_types)
{
	Assertion(tuple_types.size() >= 1, "Tuples must have more than one element!");
}
ade_type_info::ade_type_info(const char* single_type) : _type(ade_type_info_type::Simple)
{
	if (single_type != nullptr) {
		_identifier.assign(single_type);
	}

	if (single_type == nullptr) {
		// It would be better to handle this via an overload with std::nullptr_t but there are a lot of NULL usages left
		// so this is the soltuion that requires fewer changes
		_type = ade_type_info_type::Empty;
		return;
	}

	// Otherwise, make sure no one tries to specify a tuple with a single string.
	Assertion(strchr(single_type, ',') == nullptr,
		"Type string '%s' may not contain commas! Use the intializer list instead.",
		single_type);
}
ade_type_info::ade_type_info(SCP_string single_type)
	: _type(ade_type_info_type::Simple), _identifier(std::move(single_type))
{
	if (_identifier.empty()) {
		_type = ade_type_info_type::Empty;
		return;
	}

	// Otherwise, make sure no one tries to specify a tuple with a single string.
	Assertion(_identifier.find(',') == SCP_string::npos,
		"Type string '%s' may not contain commas! Use the intializer list instead.",
		_identifier.c_str());
}
ade_type_info::ade_type_info(const ade_type_array& listType)
	: _type(ade_type_info_type::Array), _elements{listType.getElementType()}
{
}
ade_type_info::ade_type_info(const ade_type_map& listType)
	: _type(ade_type_info_type::Map), _elements{listType.getKeyType(), listType.getValueType()}
{
}
ade_type_info::ade_type_info(const ade_type_iterator& iteratorType)
	: _type(ade_type_info_type::Iterator), _elements{iteratorType.getElementType()}
{
}
ade_type_info::ade_type_info(const ade_type_alternative& alternativeType)
	: _type(ade_type_info_type::Alternative), _elements(alternativeType.getElementTypes())
{
}
ade_type_info::ade_type_info(const ade_type_function& functionType) : _type(ade_type_info_type::Function) {
	_elements.reserve(functionType.getArgumentTypes().size() + 1);

	// First is return type
	_elements.push_back(functionType.getReturnType());
	// Rest are parameters
	_elements.insert(_elements.end(), functionType.getArgumentTypes().begin(), functionType.getArgumentTypes().end());
}

ade_type_info::ade_type_info(ade_type_info&&) noexcept = default;
ade_type_info& ade_type_info::operator=(ade_type_info&&) noexcept = default;

bool ade_type_info::isEmpty() const { return _type == ade_type_info_type::Empty; }
bool ade_type_info::isTuple() const { return _type == ade_type_info_type::Tuple; }
bool ade_type_info::isSimple() const { return _type == ade_type_info_type::Simple; }
bool ade_type_info::isArray() const { return _type == ade_type_info_type::Array; }
bool ade_type_info::isAlternative() const { return _type == ade_type_info_type::Alternative; }
const SCP_vector<ade_type_info>& ade_type_info::elements() const { return _elements; }
const char* ade_type_info::getIdentifier() const { return _identifier.c_str(); }
ade_type_info_type ade_type_info::getType() const { return _type; }
const ade_type_info& ade_type_info::arrayType() const { return _elements.front(); }

const SCP_string& ade_type_info::getName() const { return _name; }
void ade_type_info::setName(const SCP_string& name) { _name = name; }

ade_type_array::ade_type_array(ade_type_info elementType) : _element_type(std::move(elementType)) {}
const ade_type_info& ade_type_array::getElementType() const { return _element_type; }

ade_type_map::ade_type_map(ade_type_info keyType, ade_type_info valueType)
	: _key_type(std::move(keyType)), _value_type(std::move(valueType))
{
}
const ade_type_info& ade_type_map::getKeyType() const { return _key_type; }
const ade_type_info& ade_type_map::getValueType() const { return _value_type; }

ade_type_iterator::ade_type_iterator(ade_type_info elementType) : _element_type(std::move(elementType)) {}
const ade_type_info& ade_type_iterator::getElementType() const { return _element_type; }

ade_type_alternative::ade_type_alternative(SCP_vector<ade_type_info> elements) : _elements(std::move(elements)) {}
const SCP_vector<ade_type_info>& ade_type_alternative::getElementTypes() const { return _elements; }

ade_type_function::ade_type_function(ade_type_info returnType, SCP_vector<ade_type_info> argumentTypes)
	: _returnType(std::move(returnType)), _argumentTypes(std::move(argumentTypes))
{
}
const ade_type_info& ade_type_function::getReturnType() const { return _returnType; }
const SCP_vector<scripting::ade_type_info>& ade_type_function::getArgumentTypes() const { return _argumentTypes; }

ade_overload_list::ade_overload_list(const char* arglist) : ade_overload_list({arglist}) {}
ade_overload_list::ade_overload_list(std::initializer_list<const char*> overloads) : arg_lists(overloads) {}
ade_overload_list::ade_overload_list() : ade_overload_list({nullptr}) {}
const SCP_vector<const char*>& ade_overload_list::overloads() { return arg_lists; }
} // namespace scripting