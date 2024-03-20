// Вопрос 3.
/*
* На языке С/С++, написать функцию, которая быстрее всего (по процессорным тикам) 
* отсортирует данный ей массив чисел.
* Массив может быть любого размера со случайным порядком чисел (в том числе и отсортированным).
* Объяснить почему вы считаете, что функция соответствует заданным критериям.
*/

// Ответ.
/*
* Сортировка кучей (пирамидальная) требует гарантированно O(nLogn) операций.
*/

#include <algorithm>

void heapify(float array[], int heapSize, int rootNode)
{
	int largest = rootNode;
	int l = 2 * rootNode + 1;
	int r = 2 * rootNode + 2;

	if (l < heapSize && array[l] > array[largest])
		largest = l;

	if (r < heapSize && array[r] > array[largest])
		largest = r;

	if (largest != rootNode) 
    {
		std::swap(array[rootNode], array[largest]);
		heapify(array, heapSize, largest);
	}
}

void heapSort(float array[], int size)
{
	for (int i = size / 2 - 1; i >= 0; --i)
		heapify(array, size, i);

	for (int i = size - 1; i > 0; --i) 
    {
		std::swap(array[0], array[i]);
		heapify(array, i, 0);
	}
}