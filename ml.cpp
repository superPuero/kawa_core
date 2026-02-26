#define kw_debug

#include "kawa/core/core.h"
#include "kawa/core/managed.h"

using namespace kawa;

struct tracker
{
	static inline u32 dtor = 0;
	static inline u32 ctor = 0;
	static inline u32 copy = 0;
	static inline u32 move = 0;

	tracker() { ctor++; }
	tracker(const tracker&) { copy++; }
	tracker(tracker&&) { move++; }

	~tracker() { dtor++; }

	static void info()
	{
		kw_info("ctor {}", tracker::ctor);
		kw_info("copy {}", tracker::copy);	
		kw_info("dtor {}", tracker::dtor);
		kw_info("move {}", tracker::move);
	}
};


int main()
{
	{
		managed_array<tracker> track(100);
		track.refresh_and_keep(99);
		tracker::info();
	}
	
	tracker::info();
}

//#include <filesystem>
//#include <windows.h>
//
//using namespace kawa;
//
//enum class suite_t
//{
//	heart,
//	spade,
//	diamond,
//	club,
//
//	size_
//};
//
//enum class rank_t
//{
//	six,
//	seven,
//	eight,
//	nine,
//	ten,
//	jack,
//	queen,
//	king,
//	ace,
//
//	size_
//};
//
//string suite_to_string(suite_t suite)
//{
//	switch (suite)
//	{
//	case suite_t::heart:
//		return "heart";
//
//		break;
//	case suite_t::spade:
//		return "spade";
//
//		break;
//	case suite_t::diamond:
//		return "diamond";
//
//		break;
//	case suite_t::club:
//		return "club";
//
//		break;
//	case suite_t::size_:
//		return "err";
//
//		break;
//	default:
//		break;
//	}
//}
//
//string rank_to_string(rank_t rank)
//{
//	switch (rank)
//	{
//	case rank_t::six:
//		return "six";
//		break;
//	case rank_t::seven:
//		return "seven";
//
//		break;
//	case rank_t::eight:
//		return "eight";
//
//		break;
//	case rank_t::nine:
//		return "nine";
//
//		break;
//	case rank_t::ten:
//		return "ten";
//
//		break;
//	case rank_t::jack:
//		return "jack";
//
//		break;
//	case rank_t::queen:
//		return "queen";
//
//		break;
//	case rank_t::king:
//		return "king";
//
//		break;
//	case rank_t::ace:
//		return "ace";
//
//		break;
//	case rank_t::size_:
//		return "err";
//
//		break;
//	default:
//		break;
//	}
//}
//
//struct card_t
//{
//	rank_t rank;
//	suite_t suite;
//};
//
//
//struct card_game_master
//{
//	card_game_master(usize player_count_)
//		: players(player_count_)
//		, player_count(player_count_)
//	{
//		init_pool();
//		init_players();
//	}
//
//	void init_pool()
//	{
//		for (usize rank = 0; rank < (u32)rank_t::size_; rank++)
//		{
//			for (usize suite = 0; suite < (u32)suite_t::size_; suite++)
//			{
//				pool.emplace_back(card_t{ .rank = rank_t(rank), .suite = suite_t(suite) });
//			}
//		}
//	}
//
//	void init_players()
//	{
//		for (auto& player : players)
//		{
//			for (usize deal_count = 0; deal_count < 6; deal_count++)
//			{
//				auto rand_idx = rng::randu32() % pool.size();
//
//				card_t card = pool[rand_idx];
//				pool[rand_idx] = pool.back();
//				pool.pop_back();
//
//				player.emplace_back(card);
//			}
//		}
//	}
//
//	void next_turn()
//	{
//		dialogue_current_player();
//	}
//
//	void dialogue_current_player()
//	{
//		std::cout << "player " << current_player << " turn" << '\n';
//		
//		while (true)
//		{
//			std::cout << "choose card to play: " << '\n';
//
//			usize counter = 0;
//			for (auto& card : players[current_player])
//			{
//				std::cout << counter + 1 << ". " << rank_to_string(card.rank) << " of " << suite_to_string(card.suite) << "'s" << '\n';
//				counter++;
//			}
//
//			u32 input;
//			std::cin >> input;
//
//			card_t card = players[current_player][input - 1];
//			players[current_player][input - 1] = players[current_player].back();
//			players[current_player].pop_back();
//
//			std::cout << "you chose " << input << ": " << rank_to_string(card.rank) << " of " << suite_to_string(card.suite) << "'s" << '\n';
//
//			table.emplace_back(card);
//
//			std::cout << "do you wanna add another card? y/n" << '\n';
//
//			string y_n;
//			std::cin >> y_n;
//
//			if (y_n == "y") continue;
//			if (y_n == "n") break;
//		}
//
//	}
//
//	void defend()
//	{
//		u32 defender = current_player + 1;
//
//		std::cout << "player " << defender << " defends" << '\n';
//		std::cout << "choose card to play: " << '\n';
//
//		usize counter = 0;
//		for (auto& card : players[defender])
//		{
//			std::cout << counter + 1 << ". " << rank_to_string(card.rank) << " of " << suite_to_string(card.suite) << "'s" << '\n';
//			counter++;
//		}
//
//		u32 input;
//		std::cin >> input;
//
//		card_t card = players[defender][input - 1];
//		std::cout << "you chose " << input << ": " << rank_to_string(card.rank) << " of " << suite_to_string(card.suite) << "'s" << '\n';
//
//	}
//
//	dyn_array<card_t> pool;
//
//	dyn_array<card_t> table;
//
//	dyn_array<dyn_array<card_t>> players;
//	usize player_count = 0;
//	usize current_player = 0;
//};
//
//int main()
//{
//	card_game_master master(3);
//	master.next_turn();
//	master.defend();
//
//
//}