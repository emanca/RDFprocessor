import setuptools

with open("README.md", "r") as fh:
    long_description = fh.read()

setuptools.setup(
    name="RDFprocessor",
    version="0.0.1",
    author="Elisabetta Manca",
    author_email="elisabetta.manca@cern.ch",
    description="RDF-based mini-framework",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/emanca/RDFprocessor",
    packages=setuptools.find_packages(),
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
    ],
)
