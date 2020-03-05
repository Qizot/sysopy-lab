import random
from faker import Faker


def create_file():
    faker = Faker()

    filename = input("filename: ")
    lines = int(input("lines: "))

    file = open(filename + " .txt", "w")

    for _ in range(lines):

        line = faker.text()[:random.randint(10,30)]

        file.write(line + "\n")

    file.close()

def create_pairs():
    pairs = int(input("pairs: "))
    files = [
        "test_files/a.txt",
        "test_files/b.txt",
        "test_files/c.txt",
        "test_files/d.txt",
        "test_files/e.txt",
        "test_files/f.txt",
    ]

    first = random.choices(files, k=pairs)
    second = random.choices(files, k=pairs)

    cat = [ p[0] + ":" + p[1]  for p in zip(first, second)]
    sentence = "compare_pairs "
    sentence += " ".join(cat)
    print(sentence)

create_pairs()

    


