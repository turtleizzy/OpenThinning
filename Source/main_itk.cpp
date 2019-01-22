
#include "itkOpenThinning.h"

#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>

static const char DEFAULT_LOOKUP_TABLE_FILENAME[] = "../../Data/LookupTables/Thinning_Simple.bin";

int main(int argc, char** argv)
{
	using input_type = itk::Image<float, 3>;
	using output_type = itk::Image<unsigned char, 3>;

	if (argc == 6)
	{
		LookupTable lookuptable;
		if (!lookuptable.readFile(argv[1]))
		{
			return EXIT_FAILURE;
		}

		auto reader = itk::ImageFileReader<input_type>::New();
		reader->SetFileName(argv[2]);

		auto thinning_filter = itk::OpenThinning<input_type, output_type>::New();
		thinning_filter->SetInput(reader->GetOutput());
		thinning_filter->SetLookupTable(lookuptable);
		auto writer = itk::ImageFileWriter<output_type>::New();
		writer->SetFileName(argv[3]);
		writer->SetInput(thinning_filter->GetOutput());
		writer->Update();
		output_type::Pointer resIm = thinning_filter->GetOutput();
		auto verticeImage = itk::Image<unsigned int, 3>::New();
		verticeImage->SetRegions(resIm->GetLargestPossibleRegion());
		verticeImage->Allocate();
		unsigned short sizeX = static_cast<unsigned short>(resIm->GetLargestPossibleRegion().GetSize(0));
		unsigned short sizeY = static_cast<unsigned short>(resIm->GetLargestPossibleRegion().GetSize(1));
		unsigned short sizeZ = static_cast<unsigned short>(resIm->GetLargestPossibleRegion().GetSize(2));
		std::vector<std::array<int, 3>> vertices, edges;
		{
			#pragma omp parallel
			#pragma omp for
			for (unsigned short i=0;i<sizeX;i++)
				for (unsigned short j=0;j<sizeY;j++)
					for (unsigned short k=0;k<sizeZ;k++) {
						output_type::IndexType idx;
						idx[0] = i; idx[1] = j; idx[2] = k;
						if (resIm->GetPixel(idx)) {
							#pragma omp critical
							vertices.push_back({i, j, k});
							verticeImage->SetPixel(idx, vertices.size());
						}
					}
		}
		{
			for (const auto& v: vertices) {
				for (int d=0;d<27;d++) 
				{
					output_type::IndexType idxNew, idx;
					int totd=d, eligibleFlag=1;
					for (int i=0;i<3;i++) { 
						idx[i] = v[i]; 
						idxNew[i] = v[i] + (totd % 3) - 1; 
						totd /= 3;
						if ((idxNew[i] < 0) || (idxNew[i] >= resIm->GetLargestPossibleRegion().GetSize(i))) eligibleFlag=0;
					}
					if (!eligibleFlag || (idxNew == idx)) continue;
					if (verticeImage->GetPixel(idxNew) > 0) {
						edges.push_back({static_cast<int>(verticeImage->GetPixel(idx)), 
										 static_cast<int>(verticeImage->GetPixel(idxNew)), d});
					}
				};
			}
		}

		auto fp = std::fopen(argv[4], "w");
		std::fprintf(fp, "# vtk DataFile Version 4.2\nvtk output\nASCII\nDATASET POLYDATA\n");
		// points
		std::fprintf(fp, "POINTS %ld double\n", vertices.size());
		// coordinate system 
		int coord[3]={1, 1, 1};
		std::string itkCoord="LPS"; 
		for (int i=0;i<3;i++) 
			if (std::toupper(argv[5][i])!=itkCoord[i]) coord[i] = -1;
		for (const auto& v: vertices) {
			output_type::PointType phyPnt;
			output_type::IndexType idx;
			for (int i=0;i<3;i++) idx[i] = v[i];
			resIm->TransformIndexToPhysicalPoint(idx, phyPnt);
			std::fprintf(fp, "%.5f %.5f %.5f\n", phyPnt[0] * coord[0], phyPnt[1] * coord[1], phyPnt[2] * coord[2]);
		};
		// lines
		std::fprintf(fp, "LINES %ld %ld\n", edges.size(), edges.size()*3);
		for (const auto& e: edges) std::fprintf(fp, "2 %d %d\n", e[0]-1, e[1]-1);
		std::fclose(fp);
	}
 	else
	{
		// Print the intended usage of this program
		std::cout << "Usage: " << argv[0] << " <Lookup Table Filename> <Input Volume Filename> <Output Volume Filename> <Output VTK Filename> <Output VTK Coordinate System (LPS)>\n\n";
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
