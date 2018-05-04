////////////////////////////////////////////////////////////////////////////////
// Filename: PeonMap.h
////////////////////////////////////////////////////////////////////////////////
#pragma once

//////////////
// INCLUDES //
//////////////
#include "PeonConfig.h"
#include "PeonSystem.h"

#include <iostream>

#include <map>
#include <atomic>
#include <mutex>

/////////////
// DEFINES //
/////////////

////////////
// GLOBAL //
////////////

/*
	=> Map container, attempt 2.0

	=> Description (pt-br until I got time to translate my notes to english)

		* Introdução:

			- Continuação da versão 1.0 usando uma nova ideia (bem mais complicada).

		* Ideia:

			PS: Não cheguei a incluir um método de find() mas o método de atualização existe e é chamado de forma implicita conforma será 
			descrito abaixo.

			- Seguimos a ideia da versão 1.0 mas com alguns diferenciais, primeiro nós usaremos 2 maps internos e 2 listas, sempre estaremos
			usando o mesmo index para ler um dos maps e atualizar uma das listas.
			- Usamos várias variáveis de controle, entre elas um contador que indica quantas threads estão acessando este objeto em questão
			de forma concorrente e outra que é incrementada sempre que uma inserção ou remoção é feita que basicamente serve como um controle 
			de versão do mapa (um valor número que representa uma view).
			- Sempre que vamos adicionar e remover algum elemento, chamamos uma função auxiliar, a UpdateAndSwap().
			- A função UpdateAndSwap() no primeiro momento vai realizar um try_lock() num mutex, garantindo que periodicamente uma thread
			tenha acesso ao seguimento da função e retornando no caso de falha (antes do retorno realizamos um outro lock e unlock mas isso
			será explicado adiante).
			- Suponto que conseguimos o lock, salvamos a versão atual do map em uma variável. Usando uma variável que indica a ultima posição 
			da lista de pendência usada por esse método, nós seguimos adicionando/removendo os elementos da lista de pendência atual no OUTRO 
			map (o que não está em uso) sempre atualizando essa variável que indica a ultima posição válida para a mais recente atualizada.
			- Chegando no final da lista, verificamos se existe a necessidade de realizar o swap do index de uso atual, verificamos se a 
			diferença dos dois maps internos é muito grande (além se no mínimo temos um tamanho x), caso seja necessário, realizamos o swap.
			- Para realizar o swap, é necessário garantirmos que somos a única tread dentro do objeto, para isso antes de ler a variável 
			que indica a quantidade de threads, realizamos um lock em um outro mutex, que basicamente serve para garantir que nenhuma outra 
			thread possa avançar com uma inserção ou deleção enquanto realizamos o swap, garantindo essa unicidade e também que a versão
			do objeto ainda é a mesma (não ocorreu nenhuma inserção ou remoção após o fim de atualização do outro map) nós efetivamos o swap
			trocando o index em uso e logo liberando o mutex.
			- O map que era usado como principal antes agora passar a ser o "OUTRO" map, ele é resetado e copiamos todos os novos objetos do
			map mais atualizado para ele (lembrando que neste caso podemos fazer isso já que a leitura neste momento é thread-safe).
			- A lista antiga é zerada, desalocando todos os objetos pertencentes a ela. Ajustamos também a nova variável que indica a ultima
			posição da lista de pendencia usada para o início da nova lista em uso.
			- Liberamos por fim o primeiro mutex (aquele que foi utilizano no try_lock()) e seguimos com a operação que deveria ser realizada
			em primeiro lugar.

		* Detalhes:

			- Acabamos fazendo com que quase sempre tentemos realizar o swap, isso tem grande impacto na performance mas eu não achei uma ideia
			melhor de realizar isso até o momento.
			- Utilizamos a alocação de memória do proprio sistema, nesse caso alocações desnecessárias não são um problema.
			- Uma função de find() possivelmente irá passar pelo map interno + elementos da lista, sendo que para estes elementos é necessário 
			uma comparação um a um (aqui também existe um impacto na performance).
			- Utilizamos 1 mutex realmente bloqueante, não otimizei o código até o momento de forma a remove-lo e acredito que isso seja possível
			sim, pois definitivamente causa um grande impacto na performance.

		* Resultado:

			- Incrivelmente eu tive melhores resultados com este map do que usando um std::map + mutexes, o ganho não foi muito grande, algo em 
			torno de 50% mas geralmente varia bastante.
			- O principal ganho se mostra com a utilização de mais threads, usando 32 por exemplo o ganho foi quase de 30 vezes, aparentemente 
			essa solução escala bem numa arquitetura com muitas threads.
*/

///////////////
// NAMESPACE //
///////////////

// __InternalPeon
PeonNamespaceBegin(__InternalPeon)

////////////////////////////////////////////////////////////////////////////////
// Class name: PeonMap
////////////////////////////////////////////////////////////////////////////////
typedef uint32_t ObjectType;	// TODO: Replace this with template parameters
typedef uint32_t Key;			// TODO: Replace this with template parameters
class PeonMap
{
private:

	// The list item operation type
	enum class ItemOperation
	{
		Delete,
		Insert
	};

	// The list item type
	struct ListItem
	{
		// The key
		Key key;

		// The object
		ObjectType object;

		// The item operation
		ItemOperation operation;

		// The next list item
		std::atomic<ListItem*> next;
	};

public:
	PeonMap();
	PeonMap(const PeonMap&);
	~PeonMap();

//////////////////
// MAIN METHODS //
public: //////////

	// Insert a new element
	bool Insert(Key _key, ObjectType _object);

	// Remove an element by key
	bool Remove(Key _key);

	// Return the size
	uint32_t Size();

	// Print the entire map
	void PrintMap(bool _onlyCollisions = false);

private:

	// Insert a new item on the pendency list or check for the return indicator
	bool TryInsertItem(Key _key, ObjectType _object, ItemOperation _itemOperation, bool _returnInitialCondition, bool _returnCondition);

	// Update the other index and try to perform a swap with the current index
	void UpdateAndSwap();

///////////////
// VARIABLES //
private: //////

	// The thread-safe internal map
	std::map<Key, ListItem*, std::less<Key>, PeonAllocator<std::pair<const Key, ListItem*>>> m_InternalMap[2];

	// The pendency item list
	std::atomic<ListItem*> m_PendencyList[2];

	// The current index in use
	std::atomic<uint32_t> m_CurrentIndex;

	// The swap mutex
	std::mutex m_SwapMutex;
	std::mutex m_SwapControlMutex;

	// The last updated pendency item
	std::atomic<ListItem*>* m_LastUpdatedPendencyItem;

	// The number of threads concurrently accessing this object
	std::atomic<uint32_t> m_TotalConcurrentAccesses;

	// The object version (incremented on each insert/deletion)
	std::atomic<uint32_t> m_ObjectVersion;





	// The item count control variable
	std::atomic<uint32_t> m_ItemCount;

	// The list element count
	std::atomic<uint32_t> m_ListItemCount;

	// The count of collisions for insertions and deletions
	std::atomic<uint32_t> m_CollisionCount;
};

// __InternalPeon
PeonNamespaceEnd(__InternalPeon)
