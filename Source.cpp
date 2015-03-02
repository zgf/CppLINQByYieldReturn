#include <assert.h>
#include <iostream>
#include <experimental/generator>
#include <vector>
#include <memory>
#include <type_traits>
#include <algorithm>
#include <iterator>
#include <string>
#include <ppl.h>
#include <unordered_map>
using std::exception;
using std::pair;
using std::string;
using std::make_pair;
using std::unordered_map;
using std::unordered_multimap;
using std::experimental::generator;
using std::vector;
using std::shared_ptr;
using std::unique_ptr;
using std::make_shared;
using std::make_unique;
using std::decay;
using std::equal;
using std::forward;
using std::initializer_list;
using std::reverse_iterator;
using std::remove_reference;
using std::sort;
using std::unique;
template<typename element_type>
class LinqEnumabler
{
public:/*类型声明*/
	using value_type = element_type;
public:/*数据成员*/
	shared_ptr<generator<element_type>> container;
	unique_ptr<vector<shared_ptr<void>>> chain;
public:/*构造析构拷贝赋值函数*/
	LinqEnumabler() = delete;

	LinqEnumabler(shared_ptr<generator<value_type>> _container) :container(_container), chain(make_unique<vector<shared_ptr<void>>>())
	{
	}
	LinqEnumabler(LinqEnumabler&& _container) :container(move(_container.container)), chain(std::move(_container.chain))
	{
	}
	LinqEnumabler(shared_ptr<generator<value_type>>&& _container, unique_ptr<vector<shared_ptr<void>>>&& _chain) :container(_container), chain(move(_chain))
	{
	}
private:
	template<typename container_type>
	vector<value_type> ToVector(container_type&& container)
	{
		vector<value_type> result;
		for(auto&& element : container)
		{
			result.emplace_back(element);
		}
		return result;
	}
public:/*迭代器API*/
	auto begin()
	{
		return container->begin();
	}
	auto end()
	{
		return container->end();
	}
private:/*Generator系列函数*/

	template<typename predicate_type>
	generator<value_type> GeneratorWhere(predicate_type&& predicate)
	{
		for(auto&& element : *this)
		{
			if(predicate(element) == true)
			{
				__yield_value element;
			}
		}
	}

	template<typename selector_type>
	auto GeneratorSelect(selector_type&& selector)->generator<typename std::remove_reference<decltype(selector(element_type()))>::type>
	{
		for(auto&& element : *this)
		{
			__yield_value selector(element);
		}
	}
	//SelectMany将序列的每个元素投影到 IEnumerable<T> 并将结果序列合并为一个序列。
	template<typename selector_type>
	auto GeneratorSelectMany(selector_type&& selector)->generator<typename std::remove_reference<decltype(selector(typename element_type::value_type(), element_type()))>::type>
	{
		for(auto&& element_container : *this)
		{
			for(auto&& element : element_container)
			{
				__yield_value selector(element_container, element);
			}
		}
	}
	//将序列的每个元素投影到 IEnumerable<T>，并将结果序列合并为一个序列，并对其中每个元素调用结果选择器函数。

	template<typename collection_selector_type, typename result_selector_type>
	auto GeneratorSelectMany(collection_selector_type && collection_selector, result_selector_type&& result_selector)->generator<typename std::remove_reference<decltype(result_selector(element_type(), collection_selector(element_type())()))>::type>
	{
		for(auto&& element_container : *this)
		{
			for(auto&& element : collection_selector(element_container))
			{
				__yield_value result_selector(element_container, element);
			}
		}
	}
	/*
	//Skip<TSource>	跳过序列中指定数量的元素，然后返回剩余的元素。
	//SkipWhile<TSource>(IEnumerable<TSource>, Func<TSource, Boolean>)
	//只要满足指定的条件，就跳过序列中的元素，然后返回剩余元素。
	//*/
	template<typename predicate_type>
	generator<value_type>GeneratorSkip(const size_t count)
	{
		auto index = 0;
		for(auto&& element : *this)
		{
			if(index >= count)
			{
				__yield_value element;
			}
			++index;
		}
	}
	template<typename predicate_type>
	generator<value_type> GeneratorSkipWhile(predicate_type&& predicate)
	{
		for(auto&& element : *this)
		{
			if(!predicate(element))
			{
				__yield_value element;
			}
		}
	}
	/*
	Take<TSource>	获取序列中指定数量的元素。
	TakeWhile<TSource>(IEnumerable<TSource>, Func<TSource, Boolean>)
	只要满足指定的条件，获取序列中的元素
	*/
	template<typename predicate_type>
	generator<value_type>GeneratorTake(const size_t count)
	{
		auto index = 0;
		for(auto&& element : *this)
		{
			if(index < count)
			{
				__yield_value element;
			}
			++index;
		}
	}
	template<typename predicate_type>
	generator<value_type> GeneratorTakeWhile(predicate_type&& predicate)
	{
		for(auto&& element : *this)
		{
			if(predicate(element))
			{
				__yield_value element;
			}
		}
	}
	//Concat<TSource>	连接两个序列。
	template<typename container_type>
	generator<value_type> GeneratorConcat(container_type&& _container)
	{
		for(auto&&element : *this)
		{
			__yield_value element;
		}
		for(auto&& element : _container)
		{
			__yield_value element;
		}
	}
	//Zip<TFirst, TSecond, TResult>	将指定函数应用于两个序列的对应元素，以生成结果序列。
	template<typename container_type, typename binary_function_type>
	auto GeneratorZip(container_type&& right, binary_function_type&& binary_functor)->generator<typename remove_reference<decltype(binary_functor(value_type(), value_type()))>::type>
	{
		assert(!right.empty());
		auto&& right_iter = right.begin();
		for(auto&& left_iter = begin(); left_iter != end(); ++left_iter, ++right_iter)
		{
			__yield_value binary_functor(*left_iter, *right_iter);
		}
	}
	//Reverse<TSource>	反转序列中元素的顺序。
	generator<value_type> GeneratorReverse()
	{
		auto result = ToVector();
		auto rbegin = typename vector<value_type>::reverse_iterator(result.end());
		auto rend = typename vector<value_type>::reverse_iterator(result.begin());
		while(rbegin != rend)
		{
			__yield_value *rbegin;
			++rbegin;
		}
	}
	//Enumerable.OrderBy<TSource, TKey> 方法 (IEnumerable<TSource>, Func<TSource, TKey>, IComparer<TKey>)
	template<typename key_selector_type, typename binary_predicate_type>
	auto GeneratorOrderBy(key_selector_type&& key_selector, binary_predicate_type&& comparetor)
		->generator<typename remove_reference<decltype(comparetor(key_selector(value_type()), key_selector(value_type())))>::type>
	{
		auto result = ToVector();
		std::sort(result.begin(), result.end(), [&key_selector, &comparetor](auto&& left, auto&& right)
		{
			return comparetor(key_selector(left), key_selector(right));
		});
		for(auto&& element : result)
		{
			__yield_value result;
		}
	}
	template<typename comparetor_type>
	auto GeneratorOrderBy(comparetor_type&& comparetor)
		->generator<typename remove_reference<decltype(comparetor(value_type(), value_type()))>::type>
	{
		auto result = ToVector();
		std::sort(result.begin(), result.end(), comparetor);
		for(auto&& element : result)
		{
			__yield_value result;
		}
	}

	template<typename key_selector_type>
	auto GeneratorGroupBy(key_selector_type&& key_selector)
		->generator<typename std::remove_reference<decltype(
		make_pair(
		key_selector(element_type())
		, vector<element_type>()
		))>::type>
	{
		using key_type = typename remove_reference<decltype(key_selector(element_type()))>::type;
		unordered_map<key_type, vector<element_type>> result;
		for(auto&& element : *this)
		{
			result[key_selector(element)].emplace_back(element);
		}
		for(auto&& element : result)
		{
			__yield_value element;
		}
	}
	template<typename key_selector_type, typename value_selector_type>
	auto GeneratorGroupBy(key_selector_type&& key_selector, value_selector_type&& value_selector)
		->generator<typename std::remove_reference<decltype(
		make_pair(
		key_selector(element_type())
		, vector<typename remove_reference<
		decltype(value_selector(element_type()))>::type>()
		))>::type>
	{
		using key_type = typename remove_reference<decltype(key_selector(element_type()))>::type;
		using val_type = typename remove_reference<decltype(value_selector(element_type()))>::type;
		unordered_map<key_type, vector<val_type>> result;
		for(auto&& element : *this)
		{
			result[key_selector(element)].emplace_back(value_selector(element));
		}
		for(auto&& element : result)
		{
			__yield_value element;
		}
	}

	template<typename key_selector_type,
		typename value_selector_type,
		typename result_selector_type>
		auto GeneratorGroupBy(key_selector_type&& key_selector,
		value_selector_type&& value_selector,
		result_selector_type&& result_selector)->generator<typename remove_reference<decltype(
		result_selector(
		key_selector(element_type()),
		vector<typename remove_reference<decltype(
		value_selector(element_type()))>::type>())
		)>::type>
	{
		using key_type = typename remove_reference<decltype(key_selector(element_type()))>::type;
		using val_type = typename remove_reference<decltype(value_selector(element_type()))>::type;
		unordered_map<key_type, vector<val_type>> result;
		for(auto&& element : *this)
		{
			result[key_selector(element)].emplace_back(value_selector(element));
		}
		for(auto&& element : result)
		{
			__yield_value result_selector(element.first, element.second);
		}
		/*auto result = From(ToVector()).GroupBy(key_selector, value_selector).Select([&result_selector](auto&& element)
		{
			return result_selector(element.first, element.second);
		}).ToVector();
		for(auto&&element : result)
		{
			__yield_value element;
		}*/
	}

	/*
	public static IEnumerable<TResult> Join<TOuter, TInner, TKey, TResult>(
	this IEnumerable<TOuter> outer,
	IEnumerable<TInner> inner,
	Func<TOuter, TKey> outerKeySelector,
	Func<TInner, TKey> innerKeySelector,
	Func<TOuter, TInner, TResult> resultSelector
)*/
//生成内链接,左右key相等的拼接到一起
	template<typename container_type,
		typename outer_key_selector_type,
		typename inner_key_selector_type,
		typename result_selector_type>
		auto GeneratorJoin(container_type&& inner_container,
		outer_key_selector_type&& outer_key_selector,
		inner_key_selector_type&& inner_key_selector,
		result_selector_type&& result_selector)
		->generator<typename remove_reference<decltype(result_selector(value_type(),
		typename container_type::value_type()))>::type>
	{
		for(auto&& outer_element : *this)
		{
			for(auto&& inner_element : inner_container)
			{
				if(outer_key_selector(outer_element) == inner_key_selector(inner_element))
				{
					__yield_value result_selector(outer_element, inner_element);
				}
			}
		}
	}
	/*
	public static IEnumerable<TResult> Join<TOuter, TInner, TKey, TResult>(
	this IEnumerable<TOuter> outer,
	IEnumerable<TInner> inner,
	Func<TOuter, TKey> outerKeySelector,
	Func<TInner, TKey> innerKeySelector,
	Func<TOuter, TInner, TResult> resultSelector,
	IEqualityComparer<TKey> comparer
)
	*/
	template<typename container_type,
		typename outer_key_selector_type,
		typename inner_key_selector_type,
		typename result_selector_type,
		typename comparetor_type>
		auto GeneratorJoin(container_type&& inner_container,
		outer_key_selector_type&& outer_key_selector,
		inner_key_selector_type&& inner_key_selector,
		result_selector_type&& result_selector,
		comparetor_type&& comparetor)
		->generator<typename remove_reference<decltype(result_selector(value_type(), typename container_type::value_type()))>::type>
	{
		for(auto&& outer_element : *this)
		{
			for(auto&& inner_element : inner_container)
			{
				if(comparetor(outer_key_selector(outer_element), inner_key_selector(inner_element)))
				{
					__yield_value result_selector(outer_element, inner_element);
				}
			}
		}
	}
	/*
	public static IEnumerable<TResult> GroupJoin<TOuter, TInner, TKey, TResult>(
	this IEnumerable<TOuter> outer,
	IEnumerable<TInner> inner,
	Func<TOuter, TKey> outerKeySelector,
	Func<TInner, TKey> innerKeySelector,
	Func<TOuter, IEnumerable<TInner>, TResult> resultSelector
)
	*/
	template<typename container_type,
		typename outer_key_selector_type,
		typename inner_key_selector_type,
		typename result_selector_type>
		auto GeneratorGroupJoin(container_type&& inner_container,
		outer_key_selector_type&& outer_key_selector,
		inner_key_selector_type&& inner_key_selector,
		result_selector_type&& result_selector)
		->generator<typename remove_reference<decltype(result_selector(value_type(),
		vector<typename container_type::value_type>()))>::type>
	{
		//using collection_key_type = remove_reference<decltype(outer_key_selector(element_type))>::type;
		using key_type = value_type;
		using val_type = vector<typename container_type::value_type>;
		unordered_map<key_type, val_type> result;
		for(auto&& outer_element : *this)
		{
			if(result.find(outer_element) == result.end())
			{
				result.insert(make_pair(outer_element, val_type()));
			}
			for(auto&& inner_element : inner_container)
			{
				if(outer_key_selector(outer_element) == inner_key_selector(inner_element))
				{
					result[outer_element].emplace_back(inner_element);
				}
			}
		}
		for(auto&& element : result)
		{
			__yield_value result_selector(element.first, element.second);
		}
	}
	template<typename container_type,
		typename outer_key_selector_type,
		typename inner_key_selector_type,
		typename result_selector_type,
		typename comparetor_type>
		auto GeneratorGroupJoin(container_type&& inner_container,
		outer_key_selector_type&& outer_key_selector,
		inner_key_selector_type&& inner_key_selector,
		result_selector_type&& result_selector,
		comparetor_type&& comparetor)
		->generator<typename remove_reference<decltype(result_selector(value_type(),
		vector<typename container_type::value_type>()))>::type>
	{
		using key_type = value_type;
		using val_type = vector<typename container_type::value_type>;
		unordered_map<key_type, val_type> result;
		for(auto&& outer_element : *this)
		{
			if(result.find(outer_element) == result.end())
			{
				result.insert(make_pair(outer_element, val_type()));
			}
			for(auto&& inner_element : inner_container)
			{
				if(comparetor(outer_key_selector(outer_element), inner_key_selector(inner_element)))
				{
					result[outer_element].emplace_back(inner_element);
				}
			}
		}
		for(auto&& element : result)
		{
			__yield_value result_selector(element.first, element.second);
		}
	}

	/*
	public static IEnumerable<TSource> Distinct<TSource>(
	this IEnumerable<TSource> source
	)
	*/
	generator<value_type> GeneratorDistinct()
	{
		auto result = ToVector();
		sort(result.begin(), result.end());
		auto end = unique(result.begin(), result.end());
		for(auto iter = result.begin(); iter != end; ++iter)
		{
			__yield_value *iter;
		}
	}
	template<typename comparetor_type>
	generator<value_type> GeneratorDistinct(comparetor_type&& comparetor)
	{
		auto result = ToVector();
		sort(result.begin(), result.end());
		auto end = unique(result.begin(), result.end(), comparetor);
		for(auto iter = result.begin(); iter != end; ++iter)
		{
			__yield_value *iter;
		}
	}
	/*
	public static IEnumerable<TSource> Except<TSource>(
	this IEnumerable<TSource> first,
	IEnumerable<TSource> second
	)
	*/
	template<typename container_type>
	generator<value_type> GeneratorExcept(container_type&& right)
	{
		auto left = ToVector();
		auto right = ToVector();
		if(!std::is_sorted(left.begin(), left.end()) || !is_sorted(right.begin(), right.end()))
		{
			throw std::exception("Container need sorted");
		}
		vector<value_type> result;
		std::set_difference(left.begin(), left.end(), right.begin(), right.end(), inserter(result, result.begin()));
		for(auto&&element : result)
		{
			yield__value element;
		}
	}

	template<typename container_type, typename comparetor_type>
	generator<value_type> GeneratorExcept(container_type&& right, comparetor_type&& comparetor)
	{
		auto left = ToVector();
		auto right = ToVector();
		if(!std::is_sorted(left.begin(), left.end()) || !is_sorted(right.begin(), right.end()))
		{
			throw std::exception("Container need sorted");
		}
		vector<value_type> result;
		std::set_difference(left.begin(), left.end(), right.begin(), right.end(), inserter(result, result.begin()), comparetor);
		for(auto&&element : result)
		{
			yield__value element;
		}
	}

	/*
	public static IEnumerable<TSource> GeneratorIntersect<TSource>(
	this IEnumerable<TSource> first,
	IEnumerable<TSource> second
	)
	*/

	template<typename container_type>
	generator<value_type> GeneratorIntersect(container_type&& right)
	{
		auto left = ToVector();
		auto right = ToVector();
		if(!std::is_sorted(left.begin(), left.end()) || !is_sorted(right.begin(), right.end()))
		{
			throw std::exception("Container need sorted");
		}
		vector<value_type> result;
		std::set_intersection(left.begin(), left.end(), right.begin(), right.end(), inserter(result, result.begin()));
		for(auto&&element : result)
		{
			yield__value element;
		}
	}

	template<typename container_type, typename comparetor_type>
	generator<value_type> GeneratorIntersect(container_type&& right, comparetor_type&& comparetor)
	{
		auto left = ToVector();
		auto right = ToVector();
		if(!std::is_sorted(left.begin(), left.end()) || !is_sorted(right.begin(), right.end()))
		{
			throw std::exception("Container need sorted");
		}
		vector<value_type> result;
		std::set_intersection(left.begin(), left.end(), right.begin(), right.end(), inserter(result, result.begin()), comparetor);
		for(auto&&element : result)
		{
			yield__value element;
		}
	}

	/*
	public static IEnumerable<TSource> GeneratorIntersect<TSource>(
	this IEnumerable<TSource> first,
	IEnumerable<TSource> second
	)
	*/

	template<typename container_type>
	generator<value_type> GeneratorUnion(container_type&& right)
	{
		auto left = ToVector();
		auto right = ToVector();
		if(!std::is_sorted(left.begin(), left.end()) || !is_sorted(right.begin(), right.end()))
		{
			throw std::exception("Container need sorted");
		}
		vector<value_type> result;
		std::set_union(left.begin(), left.end(), right.begin(), right.end(), inserter(result, result.begin()));
		for(auto&&element : result)
		{
			yield__value element;
		}
	}

	template<typename container_type, typename comparetor_type>
	generator<value_type> GeneratorUnion(container_type&& right, comparetor_type&& comparetor)
	{
		auto left = ToVector();
		auto right = ToVector();
		if(!std::is_sorted(left.begin(), left.end()) || !is_sorted(right.begin(), right.end()))
		{
			throw std::exception("Container need sorted");
		}
		vector<value_type> result;
		std::set_union(left.begin(), left.end(), right.begin(), right.end(), inserter(result, result.begin()), comparetor);
		for(auto&&element : result)
		{
			yield__value element;
		}
	}
public:	/*LINQ Lazy接口API*/
	template<typename predicate_type>
	auto Where(predicate_type&& predicate)
	{
		auto shared = make_shared<generator<value_type>>(GeneratorWhere(forward<predicate_type&&>(predicate)));
		chain->emplace_back(shared);
		return LinqEnumabler(move(shared), move(chain));
	}
	template<typename selector_type>
	auto Select(selector_type&& selector)
	{
		using return_type = typename std::remove_reference<decltype(selector(value_type()))>::type;
		auto shared = make_shared<generator<return_type>>(GeneratorSelect(forward<selector_type&&>(selector)));
		chain->emplace_back(shared);
		return LinqEnumabler<return_type>(move(shared), move(chain));
	}

	//SelectMany将序列的每个元素投影到 IEnumerable<T> 并将结果序列合并为一个序列。
	template<typename selector_type>
	auto SelectMany(selector_type&& selector)
	{
		using return_type = typename std::remove_reference<decltype(selector(typename element_type::value_type(), element_type()))>::type;
		auto shared = make_shared<generator<return_type>>(GeneratorSelectMany(forward<selector_type&&>(selector)));
		chain->emplace_back(shared);
		return LinqEnumabler<return_type>(move(shared), move(chain));
	}
	template<typename collection_selector_type, typename result_selector_type>
	auto SelectMany(collection_selector_type && collection_selector, result_selector_type&& result_selector)
	{
		using return_type = typename std::remove_reference<decltype(result_selector(element_type(), collection_selector(element_type())()))>::type;
		auto shared = make_shared<generator<return_type>>(GeneratorSelectMany(forward<selector_type&&>(selector)));
		chain->emplace_back(shared);
		return LinqEnumabler<return_type>(move(shared), move(chain));
	}

	auto Skip(size_t count)
	{
		auto shared = make_shared<generator<value_type>>(GeneratorSkip(count));
		chain->emplace_back(shared);
		return LinqEnumabler(move(shared), move(chain));
	}
	template<typename predicate_type>
	auto SkipWhile(predicate_type&& predicate)
	{
		auto shared = make_shared<generator<value_type>>(GeneratorSkipWhile(forward<predicate_type&&>(predicate)));
		chain->emplace_back(shared);
		return LinqEnumabler(move(shared), move(chain));
	}
	auto Take(size_t count)
	{
		auto shared = make_shared<generator<value_type>>(GeneratorTake(count));
		chain->emplace_back(shared);
		return LinqEnumabler(move(shared), move(chain));
	}
	template<typename predicate_type>
	auto TakeWhile(predicate_type&& predicate)
	{
		auto shared = make_shared<generator<value_type>>(GeneratorTakeWhile(forward<predicate_type&&>(predicate)));
		chain->emplace_back(shared);
		return LinqEnumabler(move(shared), move(chain));
	}
	template<typename container_type>
	auto Concat(container_type&& _container)
	{
		auto shared = make_shared<generator<value_type>>(GeneratorConcat(forward<container_type&&>(_container)));
		chain->emplace_back(shared);
		return LinqEnumabler(move(shared), move(chain));
	}

	template<typename container_type, typename binary_function_type>
	auto Zip(container_type&& right, binary_function_type&& binary_functor)
	{
		using return_type = typename remove_reference<decltype(binary_functor(value_type(), value_type()))>::type;
		auto shared = make_shared<generator<return_type>>(GeneratorZip(forward<container_type&&>(right), forward<binary_function_type&&>(binary_functor)));
		chain->emplace_back(shared);
		return LinqEnumabler<return_type>(move(shared), move(chain));
	}
	auto Reverse()
	{
		auto shared = make_shared<generator<value_type>>(GeneratorReverse());
		chain->emplace_back(shared);
		return LinqEnumabler(move(shared), move(chain));
	}
	template<typename key_selector_type, typename binary_predicate_type>
	auto OrderBy(key_selector_type&& key_selector, binary_predicate_type&& comparetor)
	{
		using return_type = typename remove_reference<decltype(comparetor(key_selector(value_type()), key_selector(value_type())))>::type;
		auto shared = make_shared<generator<value_type>>(GeneratorOrderBy(forward<key_selector_type&&>(key_selector), forward <binary_predicate_type&&>(comparetor)));
		chain->emplace_back(shared);
		return LinqEnumabler<return_type>(move(shared), move(chain));
	}
	template< typename binary_predicate_type>
	auto OrderBy(binary_predicate_type&& comparetor)
	{
		using return_type = typename remove_reference<decltype(comparetor(value_type(), value_type()))>::type;
		auto shared = make_shared<generator<value_type>>(GeneratorOrderBy(forward<binary_predicate_type&&>(comparetor)));
		chain->emplace_back(shared);
		return LinqEnumabler<return_type>(move(shared), move(chain));
	}

	template<typename key_selector_type>
	auto GroupBy(key_selector_type&& key_selector)
	{
		using return_type = typename std::remove_reference<decltype(
			make_pair(
			key_selector(element_type())
			, vector<element_type>()
			))>::type;
		auto shared = make_shared<generator<return_type>>(GeneratorGroupBy(forward<key_selector_type&&>(key_selector)));
		chain->emplace_back(shared);
		return LinqEnumabler<return_type>(move(shared), move(chain));
	}

	template<typename key_selector_type, typename value_selector_type>
	auto GroupBy(key_selector_type&& key_selector, value_selector_type&& value_selector)
	{
		using key_type = typename remove_reference<decltype(key_selector(element_type()))>::type;
		using val_type = typename remove_reference<decltype(value_selector(element_type()))>::type;
		using return_type = typename std::remove_reference<decltype(make_pair(key_type(), vector<val_type>()))>::type;
		auto shared = make_shared<generator<return_type>>(GeneratorGroupBy(forward<key_selector_type&&>(key_selector), forward<value_selector_type&&>(value_selector)));
		chain->emplace_back(shared);
		return LinqEnumabler<return_type>(move(shared), move(chain));
	}
	template<typename key_selector_type, typename value_selector_type, typename result_selector_type>
	auto GroupBy(key_selector_type&& key_selector,
		value_selector_type&& value_selector,
		result_selector_type&& result_selector)
	{
		using key_type = typename remove_reference<decltype(key_selector(element_type()))>::type;
		using val_type = typename remove_reference<decltype(value_selector(element_type()))>::type;
		using return_type = typename remove_reference<decltype(result_selector(key_type(), vector<val_type>()))>::type;
		auto shared = make_shared<generator<return_type>>(GeneratorGroupBy(
			forward<key_selector_type&&>(key_selector),
			forward<value_selector_type&&>(value_selector),
			forward<result_selector_type&&>(result_selector)));
		chain->emplace_back(shared);
		return LinqEnumabler<return_type>(move(shared), move(chain));
		/*return GroupBy(key_selector, value_selector).Select([&result_selector](auto&& element)
		{
			return result_selector(element.first, element.second);
		});*/
	}

	/*
	public static IEnumerable<TResult> Join<TOuter, TInner, TKey, TResult>(
	this IEnumerable<TOuter> outer,
	IEnumerable<TInner> inner,
	Func<TOuter, TKey> outerKeySelector,
	Func<TInner, TKey> innerKeySelector,
	Func<TOuter, TInner, TResult> resultSelector
	)*/
	//生成内链接,左右key相等的拼接到一起
	template<typename container_type, typename outer_key_selector_type, typename inner_key_selector_type, typename result_selector_type>
	auto Join(container_type&& inner_container,
		outer_key_selector_type&& outer_key_selector,
		inner_key_selector_type&& inner_key_selector,
		result_selector_type&& result_selector)
	{
		using return_type = typename remove_reference<decltype(result_selector(value_type(), typename container_type::value_type()))>::type;
		auto shared = make_shared<generator<return_type>>(GeneratorJoin(
			forward<container_type&&>(inner_container),
			forward<outer_key_selector_type&&>(outer_key_selector),
			forward<inner_key_selector_type&&>(inner_key_selector),
			forward<result_selector_type&&>(result_selector)));
		chain->emplace_back(shared);
		return LinqEnumabler<return_type>(move(shared), move(chain));
	}
	/*
	public static IEnumerable<TResult> Join<TOuter, TInner, TKey, TResult>(
	this IEnumerable<TOuter> outer,
	IEnumerable<TInner> inner,
	Func<TOuter, TKey> outerKeySelector,
	Func<TInner, TKey> innerKeySelector,
	Func<TOuter, TInner, TResult> resultSelector,
	IEqualityComparer<TKey> comparer
	)
	*/
	template<typename container_type,
		typename outer_key_selector_type,
		typename inner_key_selector_type,
		typename result_selector_type,
		typename comparetor_type>
		auto Join(container_type&& inner_container,
		outer_key_selector_type&& outer_key_selector,
		inner_key_selector_type&& inner_key_selector,
		result_selector_type&& result_selector,
		comparetor_type&& comparetor)
	{
		using return_type = typename remove_reference<decltype(result_selector(value_type(), typename container_type::value_type()))>::type;
		auto shared = make_shared<generator<return_type>>(GeneratorJoin(
			forward<container_type&&>(inner_container),
			forward<outer_key_selector_type&&>(outer_key_selector),
			forward<inner_key_selector_type&&>(inner_key_selector),
			forward<result_selector_type&&>(result_selector),
			forward<comparetor_type&&>(comparetor)));
		chain->emplace_back(shared);
		return LinqEnumabler<return_type>(move(shared), move(chain));
	}
	/*
	public static IEnumerable<TResult> GroupJoin<TOuter, TInner, TKey, TResult>(
	this IEnumerable<TOuter> outer,
	IEnumerable<TInner> inner,
	Func<TOuter, TKey> outerKeySelector,
	Func<TInner, TKey> innerKeySelector,
	Func<TOuter, IEnumerable<TInner>, TResult> resultSelector
	)
	*/
	template<typename container_type,
		typename outer_key_selector_type,
		typename inner_key_selector_type,
		typename result_selector_type>
		auto GroupJoin(container_type&& inner_container,
		outer_key_selector_type&& outer_key_selector,
		inner_key_selector_type&& inner_key_selector,
		result_selector_type&& result_selector)
	{
		using return_type = typename remove_reference<decltype(result_selector(value_type(), vector<typename container_type::value_type>()))>::type;
		auto shared = make_shared<generator<return_type>>(GeneratorGroupJoin(
			forward<container_type&&>(inner_container),
			forward<outer_key_selector_type&&>(outer_key_selector),
			forward<inner_key_selector_type&&>(inner_key_selector),
			forward<result_selector_type&&>(result_selector)));
		chain->emplace_back(shared);
		return LinqEnumabler<return_type>(move(shared), move(chain));
	}
	template<typename container_type,
		typename outer_key_selector_type,
		typename inner_key_selector_type,
		typename result_selector_type,
		typename comparetor_type>
		auto GroupJoin(container_type&& inner_container,
		outer_key_selector_type&& outer_key_selector,
		inner_key_selector_type&& inner_key_selector,
		result_selector_type&& result_selector,
		comparetor_type&& comparetor)
	{
		using return_type = typename remove_reference<decltype(result_selector(value_type(), vector<typename container_type::value_type>()))>::type;
		auto shared = make_shared<generator<return_type>>(GeneratorGroupJoin(
			forward<container_type&&>(inner_container),
			forward<outer_key_selector_type&&>(outer_key_selector),
			forward<inner_key_selector_type&&>(inner_key_selector),
			forward<result_selector_type&&>(result_selector),
			forward<comparetor_type&&>(comparetor)));
		chain->emplace_back(shared);
		return LinqEnumabler<return_type>(move(shared), move(chain));
	}

	/*
	public static IEnumerable<TSource> Distinct<TSource>(
	this IEnumerable<TSource> source
	)
	*/
	auto Distinct()
	{
		using return_type = value_type;
		auto shared = make_shared<generator<return_type>>(GeneratorDistinct());
		chain->emplace_back(shared);
		return LinqEnumabler<return_type>(move(shared), move(chain));
	}
	template<typename comparetor_type>
	auto Distinct(comparetor_type&& comparetor)
	{
		using return_type = value_type;
		auto shared = make_shared<generator<return_type>>(GeneratorDistinct(forward<comparetor_type&&>(comparetor)));
		chain->emplace_back(shared);
		return LinqEnumabler<return_type>(move(shared), move(chain));
	}
	/*
	public static IEnumerable<TSource> Except<TSource>(
	this IEnumerable<TSource> first,
	IEnumerable<TSource> second
	)
	*/
	template<typename container_type>
	auto Except(container_type&& right)
	{
		using return_type = value_type;
		auto shared = make_shared<generator<return_type>>(GeneratorExcept(forward<container_type&&>(right)));
		chain->emplace_back(shared);
		return LinqEnumabler<return_type>(move(shared), move(chain));
	}

	template<typename container_type, typename comparetor_type>
	auto Except(container_type&& right, comparetor_type&& comparetor)
	{
		using return_type = value_type;
		auto shared = make_shared<generator<return_type>>(GeneratorExcept(forward<container_type&&>(right), forward<comparetor_type&&>(comparetor)));
		chain->emplace_back(shared);
		return LinqEnumabler<return_type>(move(shared), move(chain));
	}

	/*
	public static IEnumerable<TSource> GeneratorIntersect<TSource>(
	this IEnumerable<TSource> first,
	IEnumerable<TSource> second
	)
	*/

	template<typename container_type>
	auto Intersect(container_type&& right)
	{
		using return_type = value_type;
		auto shared = make_shared<generator<return_type>>(GeneratorIntersect(forward<container_type&&>(right)));
		chain->emplace_back(shared);
		return LinqEnumabler<return_type>(move(shared), move(chain));
	}

	template<typename container_type, typename comparetor_type>
	auto Intersect(container_type&& right, comparetor_type&& comparetor)
	{
		using return_type = value_type;
		auto shared = make_shared<generator<return_type>>(GeneratorIntersect(forward<container_type&&>(right), forward<comparetor_type&&>(comparetor)));
		chain->emplace_back(shared);
		return LinqEnumabler<return_type>(move(shared), move(chain));
	}

	/*
	public static IEnumerable<TSource> GeneratorIntersect<TSource>(
	this IEnumerable<TSource> first,
	IEnumerable<TSource> second
	)
	*/

	template<typename container_type>
	auto Union(container_type&& right)
	{
		using return_type = value_type;
		auto shared = make_shared<generator<return_type>>(GeneratorUnion(forward<container_type&&>(right)));
		chain->emplace_back(shared);
		return LinqEnumabler<return_type>(move(shared), move(chain));
	}

	template<typename container_type, typename comparetor_type>
	auto Union(container_type&& right, comparetor_type&& comparetor)
	{
		using return_type = value_type;
		auto shared = make_shared<generator<return_type>>(GeneratorUnion(forward<container_type&&>(right), forward<comparetor_type&&>(comparetor)));
		chain->emplace_back(shared);
		return LinqEnumabler<return_type>(move(shared), move(chain));
	}
public:/*LINQ promptly接口*/
	/*public static TSource Aggregate<TSource>(
	this IEnumerable<TSource> source,
	Func<TSource, TSource, TSource> func
)*/

	auto Aggregate()
	{
		auto init = *begin();
		auto iter = ++begin();
		for(; iter != end(); ++iter)
		{
			init = init + *iter;
		}
		return init;
	}
	template<typename binary_functor_type>
	auto Aggregate(binary_functor_type&& binary_functor)->typename std::remove_reference<decltype(binary_functor(value_type(), value_type()))>::type
	{
		auto init = *begin();
		auto iter = ++begin();
		for(; iter != end(); ++iter)
		{
			init = binary_functor(init, *iter);
		}
		return init;
	}
	template<typename init_type, typename binary_functor_type>
	init_type Aggregate(init_type&& init, binary_functor_type&& binary_functor)
	{
		for(auto&& element : *this)
		{
			init = binary_functor(init, element);
		}
		return init;
	}

	int Count()
	{
		auto count = 0;
		for(auto&&element : *this)
		{
			++count;
		}
		return count;
	}
	template<typename predicate_type>
	int Count(predicate_type&& predicate)
	{
		auto count = 0;
		for(auto&&element : *this)
		{
			if(predicate(element))
			{
				++count;
			}
		}
		return count;
	}
	value_type First()
	{
		if(begin() == end())
		{
			throw exception("Container is empty!");
		}
		return *begin();
	}
	template<typename predicate_type>
	value_type First(predicate_type&& predicate)
	{
		for(auto&& element : *this)
		{
			if(predicate(element))
			{
				return element;
			}
		}
		throw std::exception("Can't find element make predicate true");
	}
	value_type Last()
	{
		auto result = ToVector();
		if(result.empty())
		{
			throw std::exception("Container is empty!");
		}
		return result[result.size() - 1];
	}
	template<typename predicate_type>
	value_type Last(predicate_type&& predicate)
	{
		auto result = ToVector();
		auto rend = vector<value_type>::reverse_iterator(result.begin());
		auto rbegin = vector<value_type>::reverse_iterator(result.end());
		for(auto iter = rbegin; iter != rend; ++iter)
		{
			if(predicate(*iter))
			{
				return *iter;
			}
		}
		throw std::exception("Can't find element make predicate true");
	}
	value_type Max()
	{
		auto result = ToVector();
		if(result.empty())
		{
			throw std::exception("Container is empty!");
		}
		return *std::max_element(result.begin(), result.end());
	}
	template<typename predicate_type>
	value_type Max(predicate_type&& predicate)
	{
		auto result = ToVector();
		if(result.empty())
		{
			throw std::exception("Container is empty!");
		}
		return *std::max_element(result.begin(), result.end(), forward<predicate_type&&>(predicate));
	}
	value_type Min()
	{
		auto result = ToVector();
		if(result.empty())
		{
			throw std::exception("Container is empty!");
		}
		return *std::min_element(result.begin(), result.end());
	}
	template<typename predicate_type>
	value_type Min(predicate_type&& predicate)
	{
		auto result = ToVector();
		if(result.empty())
		{
			throw std::exception("Container is empty!");
		}
		return *std::min_element(result.begin(), result.end(), forward<predicate_type&&>(predicate));
	}
	template<typename predicate_type>
	value_type Single(predicate_type&& predicate)
	{
		value_type return_value;
		auto count = 0;
		for(auto&& element : *this)
		{
			if(predicate(element))
			{
				return_value = element;
				++count;
			}
			if(count > 1)
			{
				throw exception("Can't find sole element！");
			}
		}
		if(count == 0)
		{
			throw exception("Can't find element could make predicate true！");
		}
		return return_value;
	}

	template<typename container_type>
	bool SequenceEqualEqual(container_type&& target)
	{
		return equal(begin(), end(), target.begin(), target.end());
	}
	template<typename value_type>
	bool SequenceEqualEqual(const std::initializer_list<value_type>& target)
	{
		return equal(begin(), end(), target.begin(), target.end());
	}
	vector<value_type> ToVector()
	{
		vector<value_type> result;
		for(auto&& element : *this)
		{
			result.emplace_back(element);
		}
		return result;
	}
	bool Contains(value_type value)
	{
		for(auto&& element : *this)
		{
			if(value == element)
			{
				return true;
			}
			return false;
		}
	}
	value_type ElementAt(int index)
	{
		auto count = 0;
		for(auto&& element : *this)
		{
			if(count == index)
			{
				return value_type;
			}
		}
		throw std::exception("Out of range！");
	}
};

template<typename container_type>
auto GeneratorTrivial(const container_type&  container)->generator<typename container_type::value_type>
{
	for(auto&& element : container)
	{
		__yield_value element;
	}
}
template<typename container_type>
auto From(const container_type&  container)->LinqEnumabler<typename container_type::value_type>
{
	using value_type = container_type::value_type;
	auto shared = make_shared<generator<value_type>>(GeneratorTrivial(container));
	auto chain(make_unique<vector<shared_ptr<void>>>());
	chain->emplace_back(shared);
	return{ move(shared), move(chain) };
}
template<typename integral_type>
auto GeneratorRange(integral_type&& left, integral_type&& right)->generator<integral_type>
{
	static_assert(std::is_integral<integral_type>::value, "Paramenter Not Integral Type");
	for(auto i = left; i < right; ++i)
	{
		__yield_value i;
	}
}
template<typename integral_type>
auto Range(integral_type&& left, integral_type&& right)
{
	static_assert(std::is_integral<integral_type>::value, "Paramenter Not Integral Type");
	auto shared = make_shared<generator<integral_type>>(GeneratorRange(forward<integral_type&&>(left), forward<integral_type&&>(right)));
	auto chain(make_unique<vector<shared_ptr<generator<integral_type>>>>());
	chain->emplace_back(shared);
	return LinqEnumabler(move(shared), move(chain));
}
template<typename element_type, typename integral_type>
auto GeneratorRepeat(element_type&& value, integral_type&& count)->generator<element_type>
{
	static_assert(std::is_integral<integral_type>::value, "Paramenter Not Integral Type");
	for(auto i = 0; i < count; ++i)
	{
		__yield_value value;
	}
}

template<typename element_type, typename integral_type>
auto Repeat(element_type&& value, integral_type&& count)
{
	static_assert(std::is_integral<integral_type>::value, "Paramenter Not Integral Type");
	auto shared = make_shared<generator<element_type>>(GeneratorRepeat(forward<element_type&&>(value), forward<integral_type&&>(count)));
	auto chain(make_unique<vector<shared_ptr<generator<element_type>>>>());
	chain->emplace_back(shared);
	return LinqEnumabler<element_type>(move(shared), move(chain));
}

void TestGroupBy1()
{
	enum class School
	{
		PKU,
		CMU,
		CQUPT,
	};
	struct Person
	{
		string name;
		int age;
		School school;
	};
	vector<Person> list =
	{
		{string("ZGF"),20,School::CMU},
		{string("DTC"),18,School::PKU},
		{string("YH"),18,School::PKU},
		{string("LJY"),19,School::CMU},
		{string("LRJ"),17,School::CQUPT}
	};
	auto i = From(list).GroupBy([](auto&& element)
	{
		return element.age;
	}).ToVector();
}

void TestGroupBy2()
{
	enum class School
	{
		PKU,
		CMU,
		CQUPT,
	};
	struct Person
	{
		string name;
		int age;
		School school;
	};
	vector<Person> list =
	{
		{ string("ZGF"),20,School::CMU },
		{ string("DTC"),18,School::PKU },
		{ string("YH"),18,School::PKU },
		{ string("LJY"),19,School::CMU },
		{ string("LRJ"),17,School::CQUPT }
	};
	auto i = From(list).GroupBy([](auto&& element)
	{
		return element.age;
	}, [](auto&& element)
	{
		return element.name;
	}).ToVector();
}

void TestGroupBy3()
{
	enum class School
	{
		PKU,
		CMU,
		CQUPT,
	};
	struct Person
	{
		string name;
		int age;
		School school;
	};
	vector<Person> list =
	{
		{ string("ZGF"),20,School::CMU },
		{ string("DTC"),18,School::PKU },
		{ string("YH"),18,School::PKU },
		{ string("LJY"),19,School::CMU },
		{ string("LRJ"),17,School::CQUPT }
	};
	auto i = From(list).GroupBy([](auto&& element)
	{
		return element.age;
	}, [](auto&& element)
	{
		return make_pair(element.name, element.school);
	}, [](auto&& key, auto&&collection)
	{
		return make_pair(key, collection.size());
	}).ToVector();
}
int main()
{
	TestGroupBy1();
	TestGroupBy2();
	TestGroupBy3();
	vector<int> a = { 0,1,2,3,4 };
	auto i = From(a).Where([](auto&&element)->bool
	{
		return element < 3;
	}).Select([](auto&&element)
	{
		return element % 2 == 0;
	}).ToVector();


	return 0;
}


