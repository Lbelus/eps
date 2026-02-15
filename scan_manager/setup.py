from setuptools import setup

setup(
        name='doc_parser',
        version='0.1.0',
        py_modules=['doc_parser'],
        author='Lorris BELUS',
        description='A document parser with OCR capabilites that relies on various algos for token detection and yaml configuration file',
        long_description=open('README.md').read(),
        long_decription_content_type='test/markdown',
        url='',
        classifiers=[
            'Programming language :: Python :: 3',
        ],
        python_requires='>= 3.12.3',
)
