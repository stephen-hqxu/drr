#include <DisRegRep-Programme/Profiler/Splatting.hpp>

#include <DisRegRep/Container/Regionfield.hpp>
#include <DisRegRep/Container/SplattingCoefficient.hpp>

#include <DisRegRep/Core/System/ProcessThreadControl.hpp>
#include <DisRegRep/Core/View/Functional.hpp>
#include <DisRegRep/Core/Exception.hpp>
#include <DisRegRep/Core/ThreadPool.hpp>

#include <DisRegRep/RegionfieldGenerator/Base.hpp>
#include <DisRegRep/RegionfieldGenerator/ExecutionPolicy.hpp>
#include <DisRegRep/RegionfieldGenerator/VoronoiDiagram.hpp>

#include <DisRegRep/Splatting/OccupancyConvolution/Base.hpp>
#include <DisRegRep/Splatting/Base.hpp>
#include <DisRegRep/Splatting/Container.hpp>

#include <glm/common.hpp>

#include <nanobench.h>

#include <array>
#include <vector>

#include <span>
#include <string>
#include <string_view>

#include <any>
#include <tuple>

#include <algorithm>
#include <functional>
#include <iterator>
#include <ranges>

#include <future>
#include <mutex>

#include <chrono>
#include <format>
#include <memory>
#include <utility>

#include <filesystem>
#include <fstream>
#include <ios>
#include <ostream>
#include <print>

#include <exception>
#include <source_location>
#include <system_error>

#include <concepts>
#include <limits>
#include <ratio>
#include <type_traits>

#include <cassert>
#include <cstdint>

using DisRegRep::Programme::Profiler::Splatting;
namespace RfGen = DisRegRep::RegionfieldGenerator;
namespace Splt = DisRegRep::Splatting;

namespace nb = ankerl::nanobench;

namespace fs = std::filesystem;
using std::array, std::to_array, std::vector,
	std::span, std::string, std::string_view,
	std::any,
	std::tuple, std::apply, std::tuple_cat;
using std::ranges::copy, std::ranges::for_each, std::ranges::fold_left_first,
	std::invoke, std::bind_back,
	std::ostreambuf_iterator, std::back_inserter, std::make_const_iterator,
	std::views::single, std::views::repeat, std::views::transform, std::views::cartesian_product,
	std::views::join_with, std::views::zip, std::views::chunk, std::views::enumerate;
using std::ranges::range_value_t, std::ranges::range_reference_t, std::ranges::range_const_reference_t,
	std::ranges::input_range, std::ranges::view;
using std::future, std::mutex, std::unique_lock;
using std::chrono::duration,
	std::format, std::format_to, std::format_to_n,
	std::unique_ptr, std::make_unique;
using std::ostream, std::fstream, std::ofstream, std::ios_base,
	std::println;
using std::exception_ptr, std::current_exception, std::rethrow_exception, std::throw_with_nested,
	std::source_location,
	std::errc, std::make_error_code;
using std::numeric_limits;
using std::integral, std::convertible_to, std::copy_constructible,
	std::is_same_v, std::is_convertible_v,
	std::conjunction_v, std::is_invocable, std::is_invocable_v,
	std::common_type_t, std::remove_pointer_t, std::remove_cvref_t;

namespace {

using NanoBenchDefaultDuration = decltype(nb::Config::mTimeUnit);
using DefaultMemoryUnit = std::kilo;

template<typename Rg>
concept ProfilerInputRange = input_range<Rg> && view<remove_cvref_t<Rg>>;

template<typename RfGenRg>
concept RegionfieldGeneratorRange = ProfilerInputRange<RfGenRg>
	&& is_convertible_v<range_const_reference_t<RfGenRg>, const RfGen::Base* const>;

template<typename RfRg>
concept RegionfieldRange = ProfilerInputRange<RfRg>
	&& is_convertible_v<range_const_reference_t<RfRg>, const DisRegRep::Container::Regionfield* const>;

template<typename SpltRg>
concept SplattingRange = ProfilerInputRange<SpltRg>
	&& is_convertible_v<range_const_reference_t<SpltRg>, const Splt::Base* const>;

template<typename SpltRg>
concept Splatting2dRange = ProfilerInputRange<SpltRg>
	&& SplattingRange<range_reference_t<SpltRg>>;

inline constexpr auto ToSplatting2dRange = []<SplattingRange SpltRg>(SpltRg&& splatting_range) static constexpr noexcept -> auto {
	return std::forward<SpltRg>(splatting_range) | transform(single);
};

inline constexpr auto VectorMax = [](const auto dim_a, const auto dim_b) static constexpr noexcept -> auto {
	return glm::max(dim_a, dim_b);
};

template<SplattingRange SplattingBase>
[[nodiscard]] Splt::Base::DimensionType maximinRegionfieldDimension(
	SplattingBase&& splatting_base, const Splt::Base::DimensionType& extent) {
	return *fold_left_first(std::forward<SplattingBase>(splatting_base) | transform([&extent](const auto* const splat) {
		return splat->minimumRegionfieldDimension({
			.Offset = splat->minimumOffset(),
			.Extent = extent
		});
	}), VectorMax);
}

template<SplattingRange SplattingBase>
[[nodiscard]] Splt::Base::DimensionType maximinOffset(SplattingBase&& splatting_base) {
	return *fold_left_first(
		std::forward<SplattingBase>(splatting_base)
			| transform([](const auto* const splat) static { return splat->minimumOffset(); }),
		VectorMax
	);
}

[[nodiscard]] nb::Bench createBenchmark() {
	using namespace std::chrono_literals;

	nb::Bench bench;
	bench.timeUnit(1ms, "ms").unit("splat")
		.epochs(15U).minEpochTime(5ms).maxEpochTime(500ms)
		//Make some initial memory allocations, then reuse memory during benchmark so allocation time is not measured in.
		.warmup(1U)
		.performanceCounters(false)
		.output(nullptr);
	return bench;
}

//Always null-terminated.
template<integral T>
[[nodiscard]] auto toString(const T value) {
	array<string_view::value_type, numeric_limits<T>::digits10 + 1U> str {};
	format_to(str.begin(), "{}", value);
	return str;
}

}

class Splatting::Impl {
public:

	using IdentifierType = std::uint_fast32_t;

	static constexpr auto UseBuiltInRegionfield = repeat(static_cast<Container::Regionfield*>(nullptr));

	struct ExtraResult {

		using MemoryUsageType = common_type_t<
			Container::SplattingCoefficient::DenseMask::SizeType,
			Container::SplattingCoefficient::SparseMask::SizeType
		>;

		MemoryUsageType MemoryUsage;

	};

	class ExtraResultArray {
	public:

		using ArrayType = vector<ExtraResult>;
		using SizeType = ArrayType::size_type;

	private:

		ArrayType Array;

	public:

		constexpr ExtraResultArray() = default;

		ExtraResultArray(const ExtraResultArray&) = delete;

		ExtraResultArray(ExtraResultArray&&) = delete;

		ExtraResultArray& operator=(const ExtraResultArray&) = delete;

		ExtraResultArray& operator=(ExtraResultArray&&) = delete;

		constexpr ~ExtraResultArray() = default;

		constexpr void clear() noexcept {
			this->Array.clear();
		}

		void record(const Splt::Base& splatting_base, const any& memory) {
			this->Array.emplace_back(splatting_base.sizeByte(memory));
		}

		[[nodiscard]] constexpr SizeType size() const noexcept {
			return this->Array.size();
		}

		[[nodiscard]] constexpr auto view() const noexcept {
			return span(this->Array);
		}

	};

	//The implementation does not access any pointer in the regionfield generator, regionfield and splatting range,
	//	this means it is absolutely safe to have any of them being nullptr if the application does not need.
	template<RegionfieldGeneratorRange RfGenRg, RegionfieldRange RfRg, Splatting2dRange SpltRg>
	struct SubmitInfo {

		using RegionfieldGeneratorRangeType = RfGenRg;
		using RegionfieldRangeType = RfRg;
		using SplattingRangeType = SpltRg;

		using RegionfieldType = range_value_t<RegionfieldRangeType>;
		using SplattingRangeInnerType = range_value_t<SplattingRangeType>;

		RegionfieldGeneratorRangeType RegionfieldGenerator_;
		RegionfieldRangeType Regionfield;
		SplattingRangeType Splatting_;

		string_view Tag;

	};

#define IS_SUBMIT_INFO(SM_IF) is_same_v<SM_IF, SubmitInfo< \
	typename SM_IF::RegionfieldGeneratorRangeType, \
	typename SM_IF::RegionfieldRangeType, \
	typename SM_IF::SplattingRangeType \
>>

	//Regionfield generator, regionfield and splatting is an element from the corresponding range in the submit info.
	template<convertible_to<const Container::Regionfield* const> RfPtr, SplattingRange SpltRg>
	struct ProfileInfo {

		using RegionfieldPointer = RfPtr;
		using SplattingRangeType = SpltRg;

		//Uniquely identity the current profile job.
		IdentifierType Identifier;

		const RfGen::Base* RegionfieldGenerator_;
		//Any nullptr regionfield from the submit info element is replaced with one from thread cache.
		RegionfieldPointer Regionfield;
		SplattingRangeType Splatting_;

		ExtraResultArray* ExtraResultArray_;
		string_view Tag;

	};

#define IS_PROFILE_INFO(PF_IF) is_same_v<PF_IF, ProfileInfo< \
	typename PF_IF::RegionfieldPointer, \
	typename PF_IF::SplattingRangeType \
>>

private:

	static constexpr auto Delimiter = single(',');

	const class Result {
	private:

		static constexpr auto Header = to_array<string_view>({
			"variable",
			"t_median",
			"memory"
		});

	public:

		const fs::path Root;

		explicit Result(fs::path&& root) : Root(std::move(root)) {
			try {
				DRR_ASSERT(fs::is_directory(this->Root));
			} catch (const Core::Exception& e) {
				throw_with_nested(fs::filesystem_error(e.what(), this->Root, make_error_code(errc::not_a_directory)));
			}
		}

		Result(const Result&) = delete;

		Result(Result&&) = delete;

		Result& operator=(const Result&) = delete;

		Result& operator=(Result&&) = delete;

		~Result() = default;

		//Export benchmark result to a CSV file.
		template<typename PfIf>
		requires IS_PROFILE_INFO(PfIf)
		void write(const nb::Bench& bench, const PfIf& profile_info) const {
			const vector<nb::Result>& bench_result_array = bench.results();
			const ExtraResultArray& extra_result_array = *profile_info.ExtraResultArray_;
			assert(bench_result_array.size() == extra_result_array.size());

			auto result = ofstream(this->Root / format("{}.csv", profile_info.Identifier), ios_base::trunc);
			DRR_ASSERT(result);
			copy(Header | join_with(Delimiter), ostreambuf_iterator(result));
			println(result);

			for (const auto& [bench_result, extra_result] : zip(bench_result_array, extra_result_array.view())) [[likely]] {
				const nb::Config& config = bench_result.config();
				const auto [memory_usage] = extra_result;

				const auto cast_time = [target_time_unit = config.mTimeUnit](
										   const auto tick) constexpr noexcept -> NanoBenchDefaultDuration::rep {
					return NanoBenchDefaultDuration(tick) / target_time_unit;
				};

				using enum nb::Result::Measure;
				println(result, "{},{:.2f},{:.0f}",
					config.mBenchmarkName,
					cast_time(bench_result.median(elapsed)),
					memory_usage * (1.0F * DefaultMemoryUnit::den / DefaultMemoryUnit::num)
				);
			}
		}

	} Result_;
	class ResultContent {
	private:

		static constexpr IdentifierType IdentifierStart = 10000000U;

		static constexpr string_view Filename = "Content.csv";
		static constexpr auto Header = to_array<string_view>({
			"job id",
			"job title",
			"regionfield generator name",
			"splatting name",
			"container trait tag",
			"custom tag"
		});

		const fs::path Location;
		//Didn't use atomic because this is modified on the main thread only.
		IdentifierType CurrentIdentifier;

		mutable mutex Mutex;

	public:

		explicit ResultContent(const Result& result) : Location(result.Root / Filename), CurrentIdentifier(IdentifierStart) {
			auto content = ofstream(this->Location, ios_base::trunc);
			DRR_ASSERT(content);
			copy(Header | join_with(Delimiter), ostreambuf_iterator(content));
			println(content);
		}

		ResultContent(const ResultContent&) = delete;

		ResultContent(ResultContent&&) = delete;

		ResultContent& operator=(const ResultContent&) = delete;

		ResultContent& operator=(ResultContent&&) = delete;

		~ResultContent() = default;

		//Get the next result content identifier.
		[[nodiscard]] constexpr IdentifierType next() noexcept {
			return this->CurrentIdentifier++;
		}

		//Add a new index to the content.
		//It is safe to update the content with different a profile info from multiple threads.
		template<Splt::Container::IsTrait CtnTr, typename PfIf>
		requires IS_PROFILE_INFO(PfIf)
		void update(CtnTr, const nb::Bench& bench, const PfIf& profile_info) const {
			const auto [id, rf_gen, _1, splat, _2, tag] = profile_info;

			array<string_view::value_type, numeric_limits<IdentifierType>::digits10> id_str;

			using std::ranges::cbegin;
			const auto row = to_array<string_view>({
				string_view(id_str.cbegin(), make_const_iterator(format_to_n(id_str.begin(), id_str.size(), "{}", id).out)),
				bench.title(),
				rf_gen->name(),
				(*cbegin(splat))->name(),
				CtnTr::Tag,
				tag
			}) | join_with(Delimiter);

			const auto lock = unique_lock(this->Mutex);
			auto content = fstream(this->Location, ios_base::in | ios_base::out | ios_base::ate);
			DRR_ASSERT(content);
			copy(row, ostreambuf_iterator(content));
			println(content);
		}

	} ResultContent_;

	const struct {

		unique_ptr<Container::Regionfield[]> Regionfield;
		unique_ptr<ExtraResultArray[]> ExtraResultArray_;

	} ThreadCache;
	Core::ThreadPool ThreadPool;
	vector<future<void>> Future;

public:

	Impl(fs::path&& result_dir, const ThreadPoolCreateInfo& tp_info) :
		Result_(std::move(result_dir)),
		ResultContent_(this->Result_),
		ThreadCache {
			.Regionfield = make_unique<Container::Regionfield[]>(tp_info.Size),
			.ExtraResultArray_ = make_unique<ExtraResultArray[]>(tp_info.Size)
		},
		ThreadPool(tp_info.Size) {
		const auto [_, affinity_mask] = tp_info;
		DRR_ASSERT(this->ThreadPool.sizeThread() <= affinity_mask.count());
		//We need to measure time, so better the OS does not do random schedule for the threads until we are done.
		this->ThreadPool.setPriority(Core::System::ProcessThreadControl::PriorityPreset::Max);
		this->ThreadPool.setAffinityMask(affinity_mask);
	}

	Impl(const Impl&) = delete;

	Impl(Impl&&) = delete;

	Impl& operator=(const Impl&) = delete;

	Impl& operator=(Impl&&) = delete;

	~Impl() = default;

	//Generate a regionfield with automate execution policy selection.
	template<typename... Arg>
	requires is_invocable_v<const RfGen::Base, decltype(RfGen::ExecutionPolicy::SingleThreadingTrait), Arg...>
	void generateRegionfield(const RfGen::Base& rf_gen, Arg&&... arg) const {
		if (const auto generate = bind_back(std::cref(rf_gen), std::ref(std::forward<Arg>(arg))...);
			this->ThreadPool.sizeThread() == 1U) {
			//It is fine to generate regionfield in parallel if there is only one profiler thread,
			//	since it will be waiting for generation anyway, and by theory it should not cause resource contention.
			generate(RfGen::ExecutionPolicy::MultiThreadingTrait);
		} else {
			//If profiler is run in parallel, then we have to force single-thread execution
			//	to ensure generator does not contend with profiler threads.
			generate(RfGen::ExecutionPolicy::SingleThreadingTrait);
		}
	}

	//Submit an asynchronous profile job.
	template<copy_constructible Job, typename SmIf, typename... CtnTrComb>
	requires conjunction_v<is_invocable<
			Job,
			CtnTrComb,
			ProfileInfo<
				typename SmIf::RegionfieldType,
				typename SmIf::SplattingRangeInnerType
			>
		>...> && IS_SUBMIT_INFO(SmIf)
	void submit(
		Job job,
		const SmIf&& submit_info,
		const tuple<CtnTrComb...> container_trait_combination
	) {
		const auto& [rf_gen, rf, splat, tag] = submit_info;
		const auto run_job = [this, job = std::move(job), tag = string(tag)](
			const Core::ThreadPool::ThreadInfo& thread_info,
			const Splt::Container::IsTrait auto container_trait,
			const IdentifierType identifier,
			const RfGen::Base* const run_rf_gen,
			typename SmIf::RegionfieldType run_rf,
			const typename SmIf::SplattingRangeInnerType run_splat
		) -> void {
			const auto& [cache_rf, cache_extra_result] = this->ThreadCache;
			const auto [thread_idx] = thread_info;

			if (!run_rf) {
				run_rf = &cache_rf[thread_idx];
			}
			ExtraResultArray& thread_extra_result = cache_extra_result[thread_idx];
			thread_extra_result.clear();

			invoke(job, container_trait, ProfileInfo {
				.Identifier = identifier,
				.RegionfieldGenerator_ = run_rf_gen,
				.Regionfield = run_rf,
				.Splatting_ = run_splat,
				.ExtraResultArray_ = &thread_extra_result,
				.Tag = tag
			});
		};

		this->ThreadPool.enqueue(
			cartesian_product(zip(rf_gen, rf), splat) | transform([](const auto info_rg_tuple) static constexpr noexcept {
				const auto [zipped_rf, splat] = info_rg_tuple;
				return tuple_cat(zipped_rf, tuple(splat));
			}) | transform([&result_content = this->ResultContent_, container_trait_combination, &run_job](const auto info_rg_tuple) {
				return apply(
					[&result_content, &run_job, &info_rg_tuple](const auto... container_trait) {
						return tuple(apply(
							[&result_content, &run_job, container_trait](const auto&... run_rg) {
								//The result content ID assignment is unspecified
								//	because evaluation order of arguments in the outer tuple constructor is unspecified.
								return bind_back(run_job, container_trait, result_content.next(), run_rg...);
							}, info_rg_tuple)...);
					}, container_trait_combination);
			}), back_inserter(this->Future));
	}

	//Write benchmark configurations and results.
	//Make sure all pointers stored in `profile_info` are valid, if any pointer passed to submit info was invalid.
	template<typename PfIf>
	requires IS_PROFILE_INFO(PfIf)
	void writeResult(const Splt::Container::IsTrait auto container_trait, const nb::Bench& bench, const PfIf& profile_info) const {
		this->Result_.write(bench, profile_info);
		this->ResultContent_.update(container_trait, bench, profile_info);
	}

	void synchronise(ostream* const progress_log) {
		exception_ptr e_ptr;
		//WORKAROUND: A missing specialisation for indirect-value-t exposition-only alias in MSVC STL
		//	causes capturing prvalue with non-copyable inner type to fail.
		for_each(this->Future | enumerate, [progress_log, &e_ptr, total_job = this->Future.size()](auto&& i_fut) {
			auto& [i, fut] = i_fut;
			try {
				fut.get();
				if (progress_log) {
					println(*progress_log, "{}: {}/{}", source_location::current().function_name(), i, total_job);
				}
			} catch (...) {
				if (!e_ptr) [[unlikely]] {
					e_ptr = current_exception();
				}
			}
		});
		this->Future.clear();

		if (e_ptr) [[unlikely]] {
			rethrow_exception(e_ptr);
		}
	}

};

Splatting::Splatting(fs::path result_dir, const ThreadPoolCreateInfo& thread_pool_info) :
	Impl_(make_unique<Impl>(std::move(result_dir), thread_pool_info)) { }

Splatting::~Splatting() = default;

void Splatting::synchronise(ostream* const progress_log) const {
	this->Impl_->synchronise(progress_log);
}

void Splatting::sweepRadius(
	const span<const Splt::OccupancyConvolution::Base* const> splat_oc, const SizeType splat_size, const RadiusSweepInfo& info) const {
	const auto& [common_info, input] = info;
	const auto& [tag, rf_gen_info, extent] = *common_info;

	for (const auto maximin_regionfield_extent = maximinRegionfieldDimension(splat_oc, extent);
		const auto [rf_gen, rf] : apply(zip, input)) [[likely]] {
		rf->resize(maximin_regionfield_extent);
		this->Impl_->generateRegionfield(*rf_gen, *rf, *rf_gen_info);
	}

	const auto& [rf_gen, rf] = input;
	this->Impl_->submit([
		&impl = *this->Impl_,
		invoke_info = Splt::OccupancyConvolution::Base::InvokeInfo {
			.Offset = maximinOffset(splat_oc),
			.Extent = extent
		}
	](const auto container_trait, const auto&& profile_info) {
		const auto& [_1, _2, rf, splat, extra_result, _3] = profile_info;

		nb::Bench bench = createBenchmark();
		bench.title("Radius");

		for (any memory;
			const auto& current_splat : splat | Core::View::Functional::Dereference) [[likely]] {
			bench.run(toString(current_splat.Radius).data(), [&invoke_info, container_trait, rf, &memory, &current_splat] {
				nb::doNotOptimizeAway(current_splat(container_trait, invoke_info, *rf, memory));
			});
			extra_result->record(current_splat, memory);
		}
		impl.writeResult(container_trait, bench, profile_info);
	}, Impl::SubmitInfo {
		.RegionfieldGenerator_ = rf_gen,
		.Regionfield = rf | transform([](const auto* const rf_ptr) static constexpr noexcept { return rf_ptr; }),
		.Splatting_ = splat_oc | chunk(splat_size),
		.Tag = tag
	}, Splt::Container::Combination);
}

void Splatting::sweepRegionCount(const span<const Splt::Base* const> splat, const span<const RegionCountType> region_count,
	const RegionCountSweepInfo& info) const {
	const auto [common_info, input] = info;
	const auto& [tag, rf_gen_info, extent] = *common_info;

	this->Impl_->submit([
		&impl = *this->Impl_,
		region_count,
		rf_gen_info,
		invoke_info = Splt::Base::InvokeInfo {
			.Offset = maximinOffset(splat),
			.Extent = extent
		}
	](const auto container_trait, const auto&& profile_info) {
		const auto& [_1, rf_gen, rf, splat_outer, extra_result, _2] = profile_info;
		const Splt::Base& splat = *splat_outer.front();

		nb::Bench bench = createBenchmark();
		bench.title("GlobalRegionCount");

		for (any memory;
			const auto current_region_count : region_count) [[likely]] {
			rf->resize(splat.minimumRegionfieldDimension(invoke_info));
			rf->RegionCount = current_region_count;
			impl.generateRegionfield(*rf_gen, *rf, *rf_gen_info);

			bench.run(toString(current_region_count).data(), [&invoke_info, container_trait, rf, &splat, &memory] {
				nb::doNotOptimizeAway(splat(container_trait, invoke_info, *rf, memory));
			});
			extra_result->record(splat, memory);
		}
		impl.writeResult(container_trait, bench, profile_info);
	}, Impl::SubmitInfo {
		.RegionfieldGenerator_ = input,
		.Regionfield = Impl::UseBuiltInRegionfield,
		.Splatting_ = ToSplatting2dRange(splat),
		.Tag = tag
	}, Splt::Container::Combination);
}

void Splatting::sweepCentroidCount(const span<const Splt::Base* const> splat, const span<const CentroidCountType> centroid_count,
	const CentroidCountSweepInfo& info) const {
	const auto [common_info, region_count] = info;
	const auto& [tag, rf_gen_info, extent] = *common_info;

	this->Impl_->submit([
		&impl = *this->Impl_,
		centroid_count,
		region_count,
		rf_gen_info,
		invoke_info = Splt::Base::InvokeInfo {
			.Offset = maximinOffset(splat),
			.Extent = extent,
		}
	](const auto container_trait, auto profile_info) {
		const auto& [_1, _2, rf, splat_outer, extra_result, _3] = profile_info;
		const Splt::Base& splat = *splat_outer.front();

		RfGen::VoronoiDiagram voronoi_rf_gen;

		nb::Bench bench = createBenchmark();
		bench.title("LocalRegionCount");

		for (any memory;
			const auto current_centroid_count : centroid_count) [[likely]] {
			rf->resize(splat.minimumRegionfieldDimension(invoke_info));
			rf->RegionCount = region_count;
			voronoi_rf_gen.CentroidCount = current_centroid_count;
			impl.generateRegionfield(voronoi_rf_gen, *rf, *rf_gen_info);

			bench.run(toString(current_centroid_count).data(), [&invoke_info, container_trait, rf, &splat, &memory] {
				nb::doNotOptimizeAway(splat(container_trait, invoke_info, *rf, memory));
			});
			extra_result->record(splat, memory);
		}
		profile_info.RegionfieldGenerator_ = &voronoi_rf_gen;
		impl.writeResult(container_trait, bench, profile_info);
	}, Impl::SubmitInfo {
		.RegionfieldGenerator_ = single(static_cast<const RfGen::Base*>(nullptr)),
		.Regionfield = Impl::UseBuiltInRegionfield,
		.Splatting_ = ToSplatting2dRange(splat),
		.Tag = tag
	}, Splt::Container::Combination);
}