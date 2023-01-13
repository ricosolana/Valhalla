using System.Collections;
using System.Collections.Generic;
using System.IO;
using UnityEngine;

public class TestScript : MonoBehaviour
{
    // Start is called before the first frame update
    void Start()
    {
        {
            List<string> perlins = new List<string>();

            for (float y = -1.1f; y < 1.1f; y += .3f)
            {
                for (float x = -1.1f; x < 1.1f; x += .1f)
                {
                    float calc = Mathf.PerlinNoise(x, y);
                    perlins.Add(calc.ToString("0.00000000"));
                }
            }

            File.WriteAllLines("perlin_values.txt", perlins);
        }
    }
}