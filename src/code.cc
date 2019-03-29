#include "code.h"
#include <fstream>

namespace lilis
{

void t_error::f_dump() const
{
	for (auto& x : v_backtrace) x->f_print();
}

std::shared_ptr<t_location> t_at_file::f_at_head(t_pair* a_pair)
{
	auto p = dynamic_cast<t_parsed_pair*>(a_pair);
	return p ? std::make_shared<t_at_file>(p->v_path, p->v_where_head) : shared_from_this();
}

std::shared_ptr<t_location> t_at_file::f_at_tail(t_pair* a_pair)
{
	auto p = dynamic_cast<t_parsed_pair*>(a_pair);
	return p ? std::make_shared<t_at_file>(p->v_path, p->v_where_tail) : shared_from_this();
}

void t_at_file::f_print() const
{
	std::wfilebuf fb;
	fb.open(v_path, std::ios_base::in);
	v_at.f_print(v_path.c_str(), [&](long a_position)
	{
		fb.pubseekpos(a_position);
	}, [&]
	{
		return fb.sbumpc();
	});
}

void t_at_expression::f_scan(gc::t_collector& a_collector)
{
	v_expression = a_collector.f_forward(v_expression);
}

namespace
{

std::wostream& f_print_at_expression(t_object* a_expression)
{
	return std::wcerr << L"at expression" << std::endl << L'\t' << a_expression << std::endl;
}

struct t_at_list : t_at_expression
{
	t_pair* v_at;

	t_at_list(t_engine& a_engine, t_object* a_expression, t_pair* a_at) : t_at_expression(a_engine, a_expression), v_at(a_at)
	{
	}
	virtual void f_scan(gc::t_collector& a_collector)
	{
		t_at_expression::f_scan(a_collector);
		v_at = a_collector.f_forward(v_at);
	}
	void f_print_at_list(std::function<void(const t_pair*)>&& a_at_head, std::function<void(const t_pair*)>&& a_at_tail) const
	{
		f_print_at_expression(v_expression) << L'\t';
		try {
			lilis::f_dump(v_expression, {
				[&](auto x)
				{
					for (auto c : x) std::wcerr << (std::iswspace(c) ? c : L' ');
				}, std::move(a_at_head), std::move(a_at_tail)
			});
		} catch (nullptr_t) {
		}
		std::wcerr << L'^' << std::endl;
	}
};

struct t_at_head : t_at_list
{
	using t_at_list::t_at_list;
	virtual void f_print() const
	{
		f_print_at_list([&](auto x)
		{
			if (x == v_at) throw nullptr;
		},
		[&](auto)
		{
		});
	}
};

struct t_at_tail : t_at_list
{
	using t_at_list::t_at_list;
	virtual void f_print() const
	{
		f_print_at_list([&](auto)
		{
		},
		[&](auto x)
		{
			if (x == v_at) throw nullptr;
		});
	}
};

}

std::shared_ptr<t_location> t_at_expression::f_at_head(t_pair* a_pair)
{
	return std::make_shared<t_at_head>(v_engine, v_expression, a_pair);
}

std::shared_ptr<t_location> t_at_expression::f_at_tail(t_pair* a_pair)
{
	return std::make_shared<t_at_tail>(v_engine, v_expression, a_pair);
}

void t_at_expression::f_print() const
{
	f_print_at_expression(v_expression) << L"\t^" << std::endl;
}

namespace
{

template<typename T>
struct t_with_expression : t_with_value<t_object_of<t_with_expression<T>>, T>
{
	t_object* v_expression;

	t_with_expression(T* a_value, t_object* a_expression) : t_with_value<t_object_of<t_with_expression<T>>, T>(a_value), v_expression(a_expression)
	{
	}
	virtual void f_scan(gc::t_collector& a_collector)
	{
		t_with_value<t_object_of<t_with_expression<T>>, T>::f_scan(a_collector);
		v_expression = a_collector.f_forward(v_expression);
	}
};

}

t_object* t_module::t_variable::f_render(t_code& a_code, t_object* a_expression)
{
	struct t_set : t_with_expression<t_variable>
	{
		using t_with_expression<t_variable>::t_with_expression;
		virtual void f_emit(t_emit& a_emit, size_t a_stack, bool a_tail)
		{
			a_emit(e_instruction__PUSH, a_stack + 1)(this);
			v_expression->f_emit(a_emit, a_stack + 1, false);
			a_emit(a_tail ? e_instruction__CALL_TAIL : e_instruction__CALL, a_stack + 1)(1);
		}
		virtual void f_call(t_engine& a_engine, size_t a_arguments)
		{
			a_engine.v_used[-1] = v_value->v_value = *--a_engine.v_used;
		}
	};
	auto& engine = a_code.v_engine;
	return engine.f_new<t_set>(engine.f_pointer(this), engine.f_pointer(a_expression));
}

void t_module::t_variable::f_emit(t_emit& a_emit, size_t a_stack, bool a_tail)
{
	a_emit(e_instruction__PUSH, a_stack + 1)(this);
	a_emit(a_tail ? e_instruction__CALL_TAIL : e_instruction__CALL, a_stack + 1)(size_t(0));
}

void t_module::t_variable::f_call(t_engine& a_engine, size_t a_arguments)
{
	a_engine.v_used[-1] = v_value;
}

size_t t_scope::f_size() const
{
	return sizeof(t_scope) + sizeof(t_object*) * v_size;
}

void t_scope::f_scan(gc::t_collector& a_collector)
{
	v_outer = a_collector.f_forward(v_outer);
	auto p = f_locals();
	for (size_t i = 0; i < v_size; ++i) p[i] = a_collector.f_forward(p[i]);
}

size_t t_code::t_local::f_outer(t_code* a_code) const
{
	for (size_t i = 0;; ++i) {
		if (a_code == *v_value) return i;
		if (!a_code->v_outer) throw t_error("out of scope");
		a_code = *a_code->v_outer;
	}
}

t_object* t_code::t_local::f_render(t_code& a_code, t_object* a_expression)
{
	struct t_set : t_with_expression<t_local>
	{
		using t_with_expression<t_local>::t_with_expression;
		virtual void f_emit(t_emit& a_emit, size_t a_stack, bool a_tail)
		{
			v_expression->f_emit(a_emit, a_stack, false);
			a_emit(e_instruction__SET, a_stack + 1)(v_value->f_outer(a_emit.v_code))(v_value->v_index);
		}
	};
	auto& engine = a_code.v_engine;
	return engine.f_new<t_set>(engine.f_pointer(this), engine.f_pointer(a_expression));
}

void t_code::t_local::f_emit(t_emit& a_emit, size_t a_stack, bool a_tail)
{
	a_emit(e_instruction__GET, a_stack + 1)(f_outer(a_emit.v_code))(v_index);
}

void t_code::f_scan()
{
	v_this = v_engine.f_forward(v_this);
	v_outer = v_engine.f_forward(v_outer);
	v_module = v_engine.f_forward(v_module);
	for (auto& x : v_imports) x = v_engine.f_forward(x);
	for (auto& x : v_locals) x = v_engine.f_forward(x);
	v_bindings.f_scan(v_engine);
	for (auto i : v_objects) v_instructions[i] = v_engine.f_forward(static_cast<t_object*>(v_instructions[i]));
}

t_object* t_code::f_resolve(t_symbol* a_symbol, const std::shared_ptr<t_location>& a_location) const
{
	return a_location->f_try([&]
	{
		for (auto code = this;; code = *code->v_outer) {
			if (auto p = code->v_bindings.f_find(a_symbol)) return p;
			for (auto i = code->v_imports.rbegin(); i != code->v_imports.rend(); ++i)
				if (auto p = (**i)->f_find(a_symbol)) return p;
			if (!code->v_outer) throw t_error("not found");
		}
	});
}

void t_code::f_compile(const std::shared_ptr<t_location>& a_location, t_pair* a_body)
{
	t_emit emit{this};
	if (a_body) {
		auto body = v_engine.f_pointer(a_body);
		for (; body->v_tail; body = a_location->f_cast_tail<t_pair>(body)) {
			f_render(body->v_head, a_location->f_at_head(body))->f_emit(emit, 0, false);
			emit(e_instruction__POP, 0);
		}
		f_render(body->v_head, a_location->f_at_head(body))->f_emit(emit, 0, true);
	} else {
		emit(e_instruction__PUSH, 1)(static_cast<t_object*>(nullptr));
	}
	emit(e_instruction__RETURN, 0);
	for (auto& x : emit.v_labels) {
		auto p = v_instructions.data() + x.v_target;
		for (auto i : x) v_instructions[i] = p;
	}
}

t_holder<t_code>* t_code::f_new(const std::shared_ptr<t_location>& a_location, t_pair* a_pair)
{
	auto body = v_engine.f_pointer(a_location->f_cast_tail<t_pair>(a_pair));
	auto code = v_engine.f_pointer(v_engine.f_new<t_holder<t_code>>(v_engine, v_this, v_module));
	auto location = a_location->f_at_head(body);
	for (auto arguments = v_engine.f_pointer(body->v_head); arguments;) {
		auto symbol = v_engine.f_pointer(dynamic_cast<t_symbol*>(arguments.v_value));
		if (symbol) {
			(*code)->v_rest = true;
			arguments = nullptr;
		} else {
			auto pair = location->f_cast<t_pair>(arguments.v_value);
			symbol = a_location->f_cast_head<t_symbol>(pair);
			arguments = pair->v_tail;
			location = a_location->f_at_tail(pair);
			++(*code)->v_arguments;
		}
		(*code)->v_bindings.emplace(symbol, v_engine.f_new<t_local>(code, (*code)->v_locals.size()));
		(*code)->v_locals.push_back(symbol);
	}
	location = a_location->f_at_tail(body);
	(*code)->f_compile(location, body->v_tail ? location->f_cast<t_pair>(body->v_tail) : nullptr);
	return code;
}

void t_call::f_emit(t_emit& a_emit, size_t a_stack, bool a_tail)
{
	v_value->v_head->f_emit(a_emit, a_stack, false);
	auto n = a_stack;
	for (auto p = static_cast<t_pair*>(v_value->v_tail); p; p = static_cast<t_pair*>(p->v_tail)) p->v_head->f_emit(a_emit, ++n, false);
	int instruction = v_expand ? e_instruction__CALL_WITH_EXPANSION : e_instruction__CALL;
	if (a_tail) instruction += e_instruction__CALL_TAIL - e_instruction__CALL;
	a_emit(static_cast<t_instruction>(instruction), a_stack + 1)(n - a_stack);
}

}